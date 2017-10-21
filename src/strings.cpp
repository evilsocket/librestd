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
#include "strings.h"

#include <algorithm>

namespace restd {
namespace strings {

char *rtrim( char *p ){
  if( p ){
    char *trail = NULL;

    trail = strchr( p, '\r' );
    if( trail ){
      *trail = 0x00;
    }

    trail = strchr( p, '\n' );
    if( trail ){
      *trail = 0x00;
    }
  }
  return p;
}

void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
    return !std::isspace(ch);
  }));
}

void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}

void trim(std::string &s) {
  ltrim(s);
  rtrim(s);
}

std::string urldecode( const char *src ) {
  std::string dst;

  char a, b, c;
  size_t i, len = strlen(src), left = len;

  for( i = 0; i < len; ++i ) {
    c = src[i];
    left = len - i;

    if( (c == '%') && left >= 2 && ((a = src[i + 1]) && (b = src[i + 2])) && (isxdigit(a) && isxdigit(b)) ) {
      if (a >= 'a')
        a -= 'a'-'A';
      if (a >= 'A')
        a -= ('A' - 10);
      else
        a -= '0';
      if (b >= 'a')
        b -= 'a'-'A';
      if (b >= 'A')
        b -= ('A' - 10);
      else
        b -= '0';

      dst += 16*a+b;
      i   += 2;
    }
    else if( c == '+' ) {
      dst += ' ';
    } 
    else {
      dst += c;
    }
  }

  return dst;
}

}
}
