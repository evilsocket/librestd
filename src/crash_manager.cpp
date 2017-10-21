/*
 * This file is part of librestd.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@protonmail.com>
 *
 * librestd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * librestd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with librestd.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "crash_manager.h"

#include <signal.h>
#include <execinfo.h>

#include "log.h" 

namespace restd {

string crash_manager::description(int sig) {
  switch(sig)
  {
    case SIGABRT : return "ABNORMAL TERMINATION";
    case SIGFPE	 : return "FLOATING POINT EXCEPTION";
    case SIGILL	 : return "ILLEGAL INSTRUCTION";
    case SIGINT	 : return "INTERRUPT SIGNAL";
    case SIGSEGV : return "SEGMENTATION VIOLATION";
    case SIGTERM : return "TERMINATION REQUEST";
    default      : return "UNKNOWN SIGNAL";
  }
}

void crash_manager::handler(int sig) {
  if( sig == SIGTERM ) {
    log( WARNING, "Received SIGTERM, scheduling shutdown..." );
  }
  else {
    log( CRITICAL, "" );
    log( CRITICAL, "********* %s *********", description(sig).c_str() );
    log( CRITICAL, "" );

    void *trace[32];
    size_t size, i;
    char **strings;
    size    = backtrace( trace, 32 );
    strings = backtrace_symbols( trace, size );
    char used[0xFF] = {0},
         max[0xFF] = {0},
         uptime[0xFF] = {0};

    log( CRITICAL, "" );
    log( CRITICAL, "BACKTRACE:" );
    log( CRITICAL, "" );

    for( i = 0; i < size; i++ ) {
      log( CRITICAL, "  %s", strings[i] );
    }

    log( CRITICAL, "" );
    log( CRITICAL, "***************************************" );
  }

  exit(-1);
}

void crash_manager::init() {
  struct sigaction act;

  // ignore SIGHUP and SIGPIPE since we're gonna handle dead clients
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  // set SIGTERM and SIGSEGV custom handler
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  act.sa_handler = crash_manager::handler;

  sigaction( SIGTERM, &act, NULL );
  sigaction( SIGSEGV, &act, NULL );
  sigaction( SIGILL,  &act, NULL );
  sigaction( SIGFPE,  &act, NULL );
}

}
