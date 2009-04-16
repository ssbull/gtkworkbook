/* 
   thread_main.cpp - Plugin Main Thread Source File

   The GTKWorkbook Project <http://gtkworkbook.sourceforge.net/>
   Copyright (C) 2008, 2009 John Bellone, Jr. <jvb4@njit.edu>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PRACTICAL PURPOSE. See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301 USA
*/
#include <workbook/workbook.h>
#include <config/config.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <signal.h>
#include <iostream>
#include <sstream>
#include <string>
#include "concurrent/ThreadArgs.hpp"
#include "proactor/Proactor.hpp"
#include "proactor/Event.hpp"
#include "Network.hpp"
#include "CsvParser.hpp"
#include "PacketParser.hpp"
#include "Packet.hpp"

using namespace realtime;

/* @description: This method creates a filename with the prefix supplied and
   uses the pid of the process as its suffix. 
   @pre: The prefix (should be a file path, obviously). */
static std::string
append_pidname (const gchar * pre) {
  std::stringstream s;
  s << pre << getppid();
  return s.str();
}

/* @description: This is the main execution function for the thread.
   @wb: The Workbook that the thread will be changing. */
void
thread_main (ThreadArgs * args) {
  Workbook * wb = (Workbook *)args->at(0);
  Config * cfg  = (Config *)args->at(1);

  ConfigPair * logpath = cfg->get_pair (cfg, "realtime", "log", "path");
  if (IS_NULL (logpath)) {
      g_critical ("Failed loading log->path from configuration file; "
		  "exiting thread");
      return;
    }

  ConfigPair * servport = cfg->get_pair (cfg, "realtime", "tcp", "port");
  if (IS_NULL (servport)) {
      g_critical ("Failed loading tcp->port from configuration file; "
		  "exiting thread");
      return;
    }

  ConfigPair * verbosity = cfg->get_pair (cfg, 
					  "realtime", "debug", "verbosity");
  if (IS_NULL (verbosity))
    g_warning ("Failed loading debug->verbosity from configuration file.");
  
  FILE * pktlog = NULL;
  std::string logname = std::string (logpath->value).append("/");
  logname.append (append_pidname("realtime.").append(".log"));

  if ((pktlog = fopen (logname.c_str(), "w")) == NULL) {
      g_critical ("Failed opening file '%s' for packet logging; exiting"
		  " thread", logname.c_str());
      return;
    }

  /* Start up the Tcp Socket server on the port specified inside of the
     configuration file. This IS NOT a separate thread. */
  int port = atoi(servport->value);
  network::TcpServerSocket socket ( port );
  if (socket.start(5) == false) {
      g_critical ("Failed starting TcpServerSocket on port localhost:%d;"
		  " exiting thread", port);
      return;
    }

  network::TcpClientSocket client;
  if (client.connect ("127.0.0.1", 50000) == false)
    {
      g_critical ("Failed connecting to TcpClientSocket");
      return;
    }
  
  // Get a unique event identifier that will be used throughout.
  int csvEventID = proactor::Event::uniqueEventId();
  int pktEventID = proactor::Event::uniqueEventId();

  proactor::Proactor proactor;
  NetworkCsvReceiver csvDispatcher (csvEventID, &proactor);
  NetworkPktReceiver pktDispatcher (pktEventID, &proactor);
  AcceptThread acceptor (socket.newAcceptor(), &pktDispatcher);
  ConnectionThread creader (&csvDispatcher, &client);
  PacketParser packet_worker (wb, pktlog, atoi(verbosity->value));
  CsvParser csv_worker (wb, pktlog, atoi(verbosity->value));

  if (proactor.addWorker (pktEventID, &packet_worker) == false) {
      g_critical ("Failed starting packet parser worker; exiting thread.");
      return;
    }

  if (proactor.addWorker (csvEventID, &csv_worker) == false) {
      g_critical ("Failed starting csv parser worker; exiting thread.");
      return;
    }

  if (proactor.start() == false) {
      g_critical ("Failed starting Proactor; exiting thread.");
      return;
    }

  if (pktDispatcher.start() == false) {
      g_critical ("Failed starting network; exiting thread.");
      return;
    }

  if (csvDispatcher.start() == false) {
      g_critical ("Failed starting network; exiting thread.");
      return;
    }

  if (pktDispatcher.addWorker (&acceptor) == false) {
      g_critical ("Failed starting acceptor; exiting thread.");
      return;
    }

  if (csvDispatcher.addWorker (&creader) == false) {
    g_critical ("Failed starting client; exiting thread.");
    return;
  }
  
  while (!IS_NULLSTR (wb->filename)) {
      // Continually sleep basically until our application terminates.
      ::sleep (1);
    }

  FCLOSE (pktlog);

  socket.close();  

  // Interrupt threads immediately canceling them so we can quit.
  acceptor.interrupt();
  packet_worker.interrupt();
  csv_worker.interrupt();
  csvDispatcher.interrupt();
  pktDispatcher.interrupt();
  proactor.interrupt();
}

