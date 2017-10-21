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

#include <string.h>

#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;

namespace restd {

typedef std::map<std::string, std::string> headers_t;
typedef std::map<std::string, std::string> params_t;
typedef std::map<std::string, std::string> cookies_t;

typedef enum {
  GET,
  POST,
  PATCH,
  PUT,
  CONNECT
}
Method;

static char *rtrim( char *p );

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
        p = rtrim(p);
      }
      return p;
    }
};

typedef char_iterator<'&'> params_iterator;
typedef char_iterator<';'> cookies_iterator;
typedef char_iterator<'='> keyval_iterator;

class http_request 
{
  private:

    static bool parseMethodAndUri( http_request& req, line_iterator& iter );
    static bool parseHeaders( http_request& req, line_iterator& iter );
    static bool parseBody( http_request& req, line_iterator& iter, const unsigned char *buffer, size_t size );
    static bool parseParameters( http_request& req, const string& s );
    static bool parseCookies( http_request& req, const string& s );

  public:

    static const unsigned int max_size = 8192;

    Method         method;
    std::string    uri;
    std::string    path;
    std::string    version;
    std::string    host;
    headers_t      headers;
    params_t       parameters;
    cookies_t      cookies;
    std::string    body;

    inline bool has_header( const char *name ) const {
      return headers.find(name) != headers.end();
    }

    inline bool has_parameter( const char *name ) const {
      return parameters.find(name) != parameters.end();
    }

    inline string param( const char *name, const char *deflt = "" ) {
      if( has_parameter(name) ){
        return parameters[name];
      }
      return string(deflt);
    }

    static string urldecode( const char *src );
    static bool parse( http_request& req, const unsigned char *buffer, size_t size );
};

class http_response 
{
  public:

    typedef enum
    {
      HTTP_STATUS_UNINITIALIZED =       0,
      HTTP_STATUS_OK =                  200,
      HTTP_STATUS_CREATED =             201,
      HTTP_STATUS_ACCEPTED =            202,
      HTTP_STATUS_NO_CONTENT =          204,
      HTTP_STATUS_PARTIAL_CONTENTS =    206,
      HTTP_STATUS_MULTIPLE_CHOICES =    300,
      HTTP_STATUS_MOVED_PERMANENTLY =   301,
      HTTP_STATUS_MOVED_TEMPORARILY =   302,
      HTTP_STATUS_NOT_MODIFIED =        304,
      HTTP_STATUS_TEMPORARY_REDIRECT =  307,
      HTTP_STATUS_BAD_REQUEST =         400,
      HTTP_STATUS_UNAUTHORIZED =        401,
      HTTP_STATUS_FORBIDDEN =           403,
      HTTP_STATUS_NOT_FOUND =           404,
      HTTP_STATUS_INTERNAL =            500,
      HTTP_STATUS_NOT_IMPLEMENTED =     501,
      HTTP_STATUS_BAD_GATEWAY =         502,
      HTTP_STATUS_UNAVAILABLE =         503
    }
    Status;

    Status    status;
    headers_t headers;
    string    body;

    http_response( Status status_, string body_ = "", string content_type = "text/plain" );
    http_response();

    void text( string text, http_response::Status status = http_response::HTTP_STATUS_OK );
    void html( string html, http_response::Status status = http_response::HTTP_STATUS_OK );
    void json( string json, http_response::Status status = http_response::HTTP_STATUS_OK );

    std::string str();
  
  private:

    static string statusMessage( http_response::Status s );
};

}
