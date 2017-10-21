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

// string all the things!
#include <string>
#include <string.h>

namespace restd {
namespace strings {

char *rtrim( char *p );
void rtrim(std::string &s);
void ltrim(std::string &s);
void trim(std::string &s);

std::string urldecode( const char *src );

template <char SEP>
class char_iterator {
  private:

    char *ptr;
    char sep[2];

  public:

    char_iterator( const char *buffer ) : ptr( (char *)buffer ) {
      sep[0] = SEP;
      sep[1] = 0x00;
    }

    virtual char *next() {
      return strtok_r( ptr, sep, &ptr );
    }
};

class line_iterator : public char_iterator<'\n'> {
  public:
    line_iterator( const char *buffer ) : char_iterator(buffer) { }

    virtual char *next() {
      char *p = char_iterator::next();
      if( p ){
        p = strings::rtrim(p);
      }
      return p;
    }
};

}
}
