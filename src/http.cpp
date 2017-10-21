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
#include "http.h"
#include "log.h"

#include <regex>
#include <sstream>

namespace restd {

#define HTTP_END_OF_HEADERS    "\r\n\r\n"
#define HTTP_END_OF_HEADERS_SZ 4

// METHOD /URI HTTP/VERSION
static const std::regex FIRST_LINE_PARSER("([^\\s]+)\\s+([^\\s]+)\\s+HTTP.([\\d\\.]+)");  
static const std::regex PATH_QUERY_PARSER( "(/[^?]*)\\?(.+)" );
static const std::regex HEADER_PARSER("([^\\s]+)\\s*:\\s*(.+)");  

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

static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

const unsigned int http_request::max_size;

string http_request::urldecode( const char *src ) {
  string dst;

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

bool http_request::parseMethodAndUri( http_request& req, line_iterator& iter ) {
  char *line = iter.next();
  if( line == NULL ){
    log( ERROR, "Could not get first line from request." );
    return false;
  }

  string s(line);
  std::smatch m;
  if( std::regex_search( s, m, FIRST_LINE_PARSER ) == false || m.size() != 4 ) {
    log( ERROR, "Error while parsing first line from request: '%s'.", line );
    return false;
  }

  string method = m[1].str();
  if( method == "GET" ){ 
    req.method = GET; 
  }
  else if( method == "POST" ) {
    req.method = POST;
  }
  else if( method == "PATCH" ) {
    req.method = PATCH;
  }
  else if( method == "PUT" ) {
    req.method = PUT;
  }
  else if( method == "CONNECT" ) {
    req.method = CONNECT;
  }
  else {
    log( ERROR, "Invalid HTTP method '%s'", method.c_str() );
    return false;
  }

  req.uri     = m[2].str();
  req.version = m[3].str();

  log( DEBUG, "  req.method  = %s", method.c_str() );
  log( DEBUG, "  req.uri     = %s", req.uri.c_str() );

  s = req.uri;
  if( std::regex_search( s, m, PATH_QUERY_PARSER ) != false && m.size() == 3 ) {
    req.path = m[1].str();
    string params = m[2].str();
    if( parseParameters( req, params ) == false ){
      return false;
    }
  }
  else {
    req.path = req.uri;
  }

  log( DEBUG, "  req.path    = %s", req.path.c_str() );
  log( DEBUG, "  req.version = %s", req.version.c_str() );

  return true;
}

bool http_request::parseHeaders( http_request& req, line_iterator& iter ) {
  for( char *line = iter.next(); line && strlen(line); line = iter.next() ){
    string s(line);
    std::smatch m;
    if( std::regex_search( s, m, HEADER_PARSER ) == false || m.size() != 3 ) {
      log( ERROR, "Error while parsing header from request: '%s'.", line );
      return false;
    }

    string name  = m[1].str(),
           value = m[2].str();

    log( DEBUG, "  req.headers[%s] = '%s'", name.c_str(), value.c_str() );

    req.headers[name] = value;
    if( name == "Host" ){
      req.host = value;
      log( DEBUG, "    req.host = '%s'", value.c_str() );
    } else if( name == "Cookie" ) {
      if( parseCookies( req, value ) == false ) {
        // Maybe just log the error and continue?
        return false;
      }
    }
  }

  return true;
}

bool http_request::parseBody( http_request& req, line_iterator& iter, const unsigned char *buffer, size_t size ) {
  char *body = iter.next();
  if( body != NULL ){
    size_t body_size = size - ( body - (char *)buffer );
    log( DEBUG, "  req.body = (%lu bytes)", body_size );
    req.body = string( body, body_size );

    if( req.headers.find("Content-Type") != req.headers.end() && req.headers["Content-Type"] == "application/x-www-form-urlencoded" ){
      return parseParameters( req, req.body );  
    }
  }

  return true;
}

bool http_request::parseParameters( http_request& req, const string& s ) {
  params_iterator params( s.c_str() ); 

  for( char *param = params.next(); param; param = params.next() ){
    keyval_iterator kv( param );
    char *key = kv.next(),
         *val = kv.next();

    if( key && strlen(key) ){
      req.parameters[ string(key) ] = val ? urldecode(val) : string();
      log( DEBUG, "    req.params[%s] = '%s'", key, req.parameters[key].c_str() );
    }
  }

  return true;
}

bool http_request::parseCookies( http_request& req, const string& s ) {
  cookies_iterator c( s.c_str() ); 

  for( char *cookie = c.next(); cookie; cookie = c.next() ){
    keyval_iterator kv( cookie );
    char *key = kv.next(),
         *val = kv.next();

    if( key && strlen(key) ){
      string skey = key,
             sval = val ? val : string();

      trim(sval);
      trim(skey);

      req.cookies[skey] = urldecode(sval.c_str());
      log( DEBUG, "    req.cookies[%s] = '%s'", skey.c_str(), req.cookies[skey].c_str() );
    }
  }

  return true;
}

bool http_request::parse( http_request& req, const unsigned char *buffer, size_t size ) {
  line_iterator iter( (const char *)buffer );  

  req.raw = string( (const char *)buffer, size );

  if( parseMethodAndUri( req, iter ) == false ){
    return false;
  }
  else if( parseHeaders( req, iter ) == false ){
    return false;
  }
  else if( parseBody( req, iter, buffer, size ) == false ){
    return false;
  }

  return true;
}

string http_response::statusMessage( http_response::Status s ) {
  switch(s) { 
    case HTTP_STATUS_OK: return "Ok";
    case HTTP_STATUS_CREATED: return "Created";
    case HTTP_STATUS_ACCEPTED: return "Accepted";
    case HTTP_STATUS_NO_CONTENT: return "No Content";
    case HTTP_STATUS_PARTIAL_CONTENTS: return "Partial Contents";
    case HTTP_STATUS_MULTIPLE_CHOICES: return "Multiple Choices";
    case HTTP_STATUS_MOVED_PERMANENTLY: return "Moved";
    case HTTP_STATUS_MOVED_TEMPORARILY: return "Moved";
    case HTTP_STATUS_NOT_MODIFIED: return "Not Modified";
    case HTTP_STATUS_TEMPORARY_REDIRECT: return "Redirect";
    case HTTP_STATUS_BAD_REQUEST: return "Bad Request";
    case HTTP_STATUS_UNAUTHORIZED: return "Unauthorized";
    case HTTP_STATUS_FORBIDDEN: return "Forbidden";
    case HTTP_STATUS_NOT_FOUND: return "Not Found";
    case HTTP_STATUS_INTERNAL: return "Internal Error";
    case HTTP_STATUS_NOT_IMPLEMENTED: return "Not Implemented";
    case HTTP_STATUS_BAD_GATEWAY: return "Bad Gateway";
    case HTTP_STATUS_UNAVAILABLE: return "Unavailable";
  }

  return "Unknown";
}

http_response::http_response() : status(HTTP_STATUS_OK) {

}

http_response::http_response( Status status_, string body_ /* = "" */, string content_type /* = "text/plain" */  ) :
  status(status_), body(body_) {
  if( !content_type.empty() ){
    headers["Content-Type"] = content_type;
  }
}

void http_response::text( string text, http_response::Status status /* = http_response::HTTP_STATUS_OK */ ) {
  status = status;
  body   = text;
  headers["Content-Type"] = "text/plain";
}

void http_response::html( string html, http_response::Status status /* = http_response::HTTP_STATUS_OK */ ) {
  status = status;
  body   = html;
  headers["Content-Type"] = "text/html; charset=utf-8";
}

void http_response::json( string json, http_response::Status status /* = http_response::HTTP_STATUS_OK */ ) {
  status = status;
  body   = json;
  headers["Content-Type"] = "application/json";
}

std::string http_response::str() {
  std::stringstream ss;

  ss << "HTTP/1.1 " << (int)status << " " << statusMessage(status) << "\r\n";
  
  for( auto i = headers.begin(), e = headers.end(); i != e; ++i ){
    ss << i->first << ": " << i->second << "\r\n";
  }

  if( headers.find("Server") == headers.end() ){
    ss << "Server: librestd\r\n";
  }

  if( headers.find("Content-Length") == headers.end() && !body.empty() ){
    ss << "Content-Length: " << body.size() << "\r\n";
  }

  if( headers.find("Connection") == headers.end() ){
    ss << "Connection: close\r\n";
  }

  ss << "\r\n";

  if( body.empty() ){
    ss << "\r\n";
  }
  else {
    ss << body;
  }

  return ss.str();
}

}
