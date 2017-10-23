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

#include "strings.h"
#include "json.hpp"

#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;

namespace restd {

using json = nlohmann::json;

typedef std::map<std::string, std::string> headers_t;
typedef std::map<std::string, std::string> params_t;
typedef std::map<std::string, std::string> cookies_t;

typedef enum {
  GET     = 1 << 0,
  POST    = 1 << 1,
  PATCH   = 1 << 2,
  PUT     = 1 << 3,
  CONNECT = 1 << 4,

  ANY     = 0xffff
}
Method;

typedef strings::char_iterator<'&'> params_iterator;
typedef strings::char_iterator<';'> cookies_iterator;
typedef strings::char_iterator<'='> keyval_iterator;

class http_request 
{
  private:

    static bool parseMethodAndUri( http_request& req, strings::line_iterator& iter );
    static bool parseHeaders( http_request& req, strings::line_iterator& iter );
    static bool parseBody( http_request& req, strings::line_iterator& iter, const unsigned char *buffer, size_t size );
    static bool parseCookies( http_request& req, const string& s );

    // Body parsers.
    static bool parseUrlencodedFormParameters( http_request& req, const string& s );
    static bool parseJson( http_request& req, const string& s );

  public:

    static const unsigned int max_size = 8192;

    std::string    raw;

    Method         method;
    std::string    uri;
    std::string    path;
    std::string    version;
    std::string    host;
    headers_t      headers;
    params_t       parameters;
    cookies_t      cookies;
    std::string    body;
    nlohmann::json json;

    inline string method_name() const {
      switch(method) {
        case GET:     return "GET";
        case POST:    return "POST";
        case PATCH:   return "PATCH";
        case PUT:     return "PUT";
        case CONNECT: return "CONNECT";
      }
      return "???";
    }

    inline bool has_header( const char *name ) const {
      return headers.find(name) != headers.end();
    }

    inline bool header_is( const char *name, const char *value ) {
      if( has_header(name) ) {
        return ( headers[name] == value );
      }

      return false;
    }

    inline bool is_json() {
      return header_is( "Content-Type", "application/json" );
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

    void bad_request();
    void not_found();

    void text( string text, http_response::Status status = http_response::HTTP_STATUS_OK );
    void html( string html, http_response::Status status = http_response::HTTP_STATUS_OK );
    void json( string json, http_response::Status status = http_response::HTTP_STATUS_OK );

    std::string str();
  
  private:

    static string statusMessage( http_response::Status s );
};

}
