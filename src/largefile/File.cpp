/* 
   The GTKWorkbook Project <http://gtkworkbook.sourceforge.net/>
   Copyright (C) 2009 John Bellone, Jr. <jvb4@njit.edu>

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
#include "File.hpp"
#include <iostream>
#include <concurrent/ScopedMemoryLock.hpp>
#include <cstdio>

namespace largefile {

	FileDispatcher::FileDispatcher (int e, proactor::Proactor * pro) {
		this->fp = NULL;
		this->pro = pro;
		setEventId(e);
	}

	FileDispatcher::~FileDispatcher (void) {
		if (this->fp != NULL)
			this->close();
	}

	void
	FileDispatcher::read (long int start, long int N) {
		// WARNING: Need some form of a lock on this method.
		LineReader * reader = new LineReader (this, this->fp, start, N);
		this->addWorker (reader);
	}

	void
	FileDispatcher::index (void) {
		LineIndexer * indexer = new LineIndexer (this, this->fp, this->marks);
		this->addWorker (indexer);
	}

	bool
	FileDispatcher::open (const std::string & filename) {
		if (filename.length() == 0)
			return false;

		if ((this->fp = std::fopen (filename.c_str(), "r")) == NULL) {
			// stub: throw an error somewhere
			return false;
		}

		std::fseek (this->fp, 0, SEEK_END);
		long int byte_end = std::ftell (this->fp);

		for (int ii = 0; ii < 101; ii++) {
			float N = ii, K = 10000;
			this->marks[ii] = (long int)((N/K) * byte_end);
		}

		std::fseek (this->fp, 0, SEEK_SET);
		
		concurrent::ScopedMemoryLock::addMemoryLock ((unsigned long int)this->fp);
		this->filename = filename;
		return true;
	}

	bool 
	FileDispatcher::close (void) {
		concurrent::ScopedMemoryLock mutex ((unsigned long int)this->fp);
		if (this->fp == NULL)
			return false;

		std::fclose (this->fp); this->fp = NULL;
		mutex.remove();
		return true;
	}
  
	void *
	FileDispatcher::run (void * null) {
		this->running = true;

		while (this->running == true) {
			if (this->fp == NULL) {
				this->running = false;
				break;
			}

			this->inputQueue.lock();
      
			while (this->inputQueue.size() > 0) {
				if (this->running == false)
					break;

				this->pro->onReadComplete (this->inputQueue.pop());
			}

			this->inputQueue.unlock();

			Thread::sleep (5);
		}

		return NULL;
	}
  
	LineIndexer::LineIndexer (proactor::InputDispatcher * d,
									  FILE * fp,
									  long int * marks) {
		this->fp = fp;
		this->dispatcher = d;
		this->marks = marks;
	}

	LineIndexer::~LineIndexer (void) {
	}

	void *
	LineIndexer::run (void * null) {
		this->running = true;

		concurrent::ScopedMemoryLock mutex ((unsigned long int)this->fp, true);
		
		long int start = std::ftell (this->fp), cursor = 1;
		
		// Take the relative byte position, e.g. .75 * byte_end, and seek backwards until
		// we get a newline. We now have the line at that relative byte position. Set it
		// to the marks array so that we can quick line seek.
		while (!feof (this->fp) && (cursor < 101)) {
			int c = 0, ii = 0;
			std::fseek(this->fp, this->marks[cursor], SEEK_SET);

			while ((c = fgetc (this->fp)) != EOF) {
				if ((c == '\r') || (c=='\n')) break;
				std::fseek(this->fp, this->marks[cursor] - (++ii) - 1, SEEK_CUR); 
			}

			this->marks[cursor] = (this->marks[cursor] - ii);
			cursor++;
			clearerr(this->fp);
		}

		std::fseek (this->fp, start, SEEK_SET);
		
		this->dispatcher->removeWorker (this);
		return NULL;
	}

	LineReader::LineReader (proactor::InputDispatcher * d,
									FILE * fp,
									long int start,
									long int N) {
		this->fp = fp;
		this->dispatcher = d;
		this->startOffset = start;
		this->numberOfLinesToRead = N;
	}

	LineReader::~LineReader (void) {
	}

	void *
	LineReader::run (void * null) {
		this->running = false;
		char buf[4096];

		concurrent::ScopedMemoryLock mutex ((unsigned long int)this->fp, true);

		// Record current position and seek to where we're going to start.
		long int & read_max = this->numberOfLinesToRead;
		std::fseek (this->fp, this->startOffset, SEEK_SET);

		for (long int ii = 0; ii < read_max; ii++) {
			if (std::fgets (buf, 4096, this->fp) == NULL)		
				break;
      
			this->dispatcher->onReadComplete (std::string (buf));
		}

		this->running = false;
		this->dispatcher->removeWorker (this);
		return NULL;
	}

} // end of namespace
