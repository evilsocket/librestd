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
#include "log.h"

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <string>
#include <mutex>

using std::string;
using std::mutex;

namespace restd {

static log_level_t __level      = INFO;
static FILE       *__fp         = stdout;
static string      __dateformat = "%m/%d/%Y %X";
static mutex       __mutex;

void set_log_level( log_level_t level ) {
  __level = level;
}

void set_log_fp( FILE *fp ) {
  __fp = fp;
}

void set_log_dateformat( const char *format ) {
  __dateformat = format;
}

void log( log_level_t level, const char *format, ... ) {
  char 	buffer[4096] 	= {0},
        timestamp[0xFF] = {0};
  string slevel = "???";
  va_list ap;
  time_t 		rawtime  = 0;
  struct tm * timeinfo = NULL;

  if( level >= __level ) {
    std::unique_lock<std::mutex> lock(__mutex);

    va_start( ap, format );
    vsnprintf( buffer, 4096, format, ap );
    va_end(ap);

    time( &rawtime );
    timeinfo = localtime( &rawtime );

    strftime( timestamp, 0xFF, __dateformat.c_str(), timeinfo );

    switch( level ){
      case DEBUG    : slevel = "DBG"; break;
      case WARNING  : slevel = "WAR"; break;
      case INFO     : slevel = "INF"; break;
      case ERROR    : slevel = "ERR"; break;
      case CRITICAL : slevel = "CRT"; break;
    }

    fprintf( __fp, "[%s] [%s] %s\n", timestamp, slevel.c_str(), buffer );
  }
}

}
