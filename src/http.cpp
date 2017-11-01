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

#define HTTP_SERVER_SOFTWARE   "librestd/1.0"

// METHOD /URI HTTP/VERSION
static const std::regex kFirstLineParser("([^\\s]+)\\s+([^\\s]+)\\s+HTTP.([\\d\\.]+)");  
// /PATH?QUERY
static const std::regex kPathQueryParser( "(/[^?]*)\\?(.+)" );
// KEY: VALUE
static const std::regex kHeaderParser("([^\\s]+)\\s*:\\s*(.+)");  

const unsigned int http_request::max_size;
const unsigned int http_request::read_timeout;

bool http_request::parseMethodAndUri( http_request& req, strings::line_iterator& iter ) {
  char *line = iter.next();
  if( line == NULL ){
    log( ERROR, "Could not get first line from request." );
    return false;
  }

  string s(line);
  std::smatch m;
  if( std::regex_search( s, m, kFirstLineParser ) == false || m.size() != 4 ) {
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
  if( std::regex_search( s, m, kPathQueryParser ) != false && m.size() == 3 ) {
    req.path = m[1].str();
    string params = m[2].str();
    if( parseUrlencodedFormParameters( req, params ) == false ){
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

bool http_request::parseHeaders( http_request& req, strings::line_iterator& iter ) {
  for( char *line = iter.next(); line && strlen(line); line = iter.next() ){
    string s(line);
    std::smatch m;
    if( std::regex_search( s, m, kHeaderParser ) == false || m.size() != 3 ) {
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
    }
    else if( name == "Cookie" ) {
      if( parseCookies( req, value ) == false ) {
        // Maybe just log the error and continue?
        return false;
      }
    }
  }

  return true;
}

bool http_request::parseBody( http_request& req, strings::line_iterator& iter, const unsigned char *buffer, size_t size ) {
  char *body = iter.next();
  if( body != NULL ){
    size_t body_size = size - ( body - (char *)buffer );
    log( DEBUG, "  req.body = (%lu bytes)", body_size );
    req.body = string( body, body_size );

    if( req.has_header("Content-Type") ) {
      string content_type = req.headers["Content-Type"];

      if( content_type == "application/x-www-form-urlencoded" ){
        return parseUrlencodedFormParameters( req, req.body );  
      } 
      else if( content_type == "application/json" ){
        return parseJson( req, req.body );
      }
      else {
        log( DEBUG, "Unhandled content type '%s'.", content_type.c_str() );
      }
    }
  }

  return true;
}

bool http_request::parseUrlencodedFormParameters( http_request& req, const string& s ) {
  params_iterator params( s.c_str() ); 

  for( char *param = params.next(); param; param = params.next() ){
    keyval_iterator kv( param );
    char *key = kv.next(),
         *val = kv.next();

    if( key && strlen(key) ){
      req.parameters[ string(key) ] = val ? strings::urldecode(val) : string();
      log( DEBUG, "    req.params[%s] = '%s'", key, req.parameters[key].c_str() );
    }
  }

  return true;
}

bool http_request::parseJson( http_request& req, const string& s ) {
  try {
    req.json = json::parse( req.body );
    return true;
  }
  catch( const std::invalid_argument& e ) {
    log( ERROR, "Error while parsing application/json request: %s", e.what() );
    return false;
  }
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

      strings::trim(sval);
      strings::trim(skey);

      req.cookies[skey] = strings::urldecode(sval.c_str());
      log( DEBUG, "    req.cookies[%s] = '%s'", skey.c_str(), req.cookies[skey].c_str() );
    }
  }

  return true;
}

bool http_request::parse( http_request& req, const unsigned char *buffer, size_t size ) {
  strings::line_iterator iter( (const char *)buffer );  

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

void http_response::bad_request() {
  status = http_response::HTTP_STATUS_BAD_REQUEST;
  body   = "Bad Request";
  headers["Content-Type"] = "text/plain; charset=utf-8";
}

void http_response::not_found() {
  status = http_response::HTTP_STATUS_NOT_FOUND;
  body   = "Not Found ¯\\_(ツ)_/¯";
  headers["Content-Type"] = "text/plain; charset=utf-8";
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
    ss << "Server: " << HTTP_SERVER_SOFTWARE << "\r\n";
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
