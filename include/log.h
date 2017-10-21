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
#pragma once

#include <stdio.h>

namespace restd {

typedef enum {
	DEBUG = 0,
	INFO,
	WARNING,
	ERROR,
	CRITICAL
}
log_level_t;

void set_log_level( log_level_t level );
void set_log_fp( FILE *fp );
void set_log_dateformat( const char *format );

void log( log_level_t level, const char *format, ... );

}
