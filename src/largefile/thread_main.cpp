/* 
   thread_main.cpp - Largefile Main Thread Source File

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
#include <concurrent/Thread.hpp>
#include <concurrent/ThreadArgs.hpp>
#include <concurrent/SharedMemoryLock.hpp>
#include <proactor/Proactor.hpp>
#include <proactor/Event.hpp>
#include <gtkextra/gtksheet.h>
#include <fstream>
#include "File.hpp"
#include <iostream>
#include <string>
#include <sstream>

using namespace largefile;

void
thread_main (ThreadArgs * args) {
  Workbook * wb = (Workbook *)args->at(0);
  Config * cfg  = (Config *)args->at(1);
  gboolean * SHUTDOWN = (gboolean *)args->at(2);
  GtkSheet * sheet = (GtkSheet *)wb->sheet_first->gtk_sheet;

  proactor::Proactor proactor;
  FileDispatcher fd (&proactor);

  while (*SHUTDOWN == FALSE) {
    
    // Continually sleep basically until our application terminates.
    concurrent::Thread::sleep (100);
  }

  delete args;
}
