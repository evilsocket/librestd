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
static const std::regex kFirstLineParser("([^\\s]+)\\s+([^\\s]+)\\s+HTTP.([\\d\\.]+)\r\n");  
// /PATH?QUERY
static const std::regex kPathQueryParser( "(/[^?]*)\\?(.+)" );
// KEY: VALUE
static const std::regex kHeaderParser("([^\\s]+)\\s*:\\s*(.+)\r\n");  

const unsigned int http_request::chunk_size;
const unsigned int http_request::read_timeout;

http_request::http_request() : parser_state(PARSE_BEGIN), content_length(0) {

}

bool http_request::parse_json( const string& s ) {
  try {
    this->json = json::parse( s );
    return true;
  }
  catch( const std::invalid_argument& e ) {
    log( ERROR, "Error while parsing application/json request: %s", e.what() );
    return false;
  }
}

bool http_request::parse_query( const string& s ) {
  params_iterator params( s.c_str() ); 

  for( char *param = params.next(); param; param = params.next() ){
    keyval_iterator kv( param );
    char *key = kv.next(),
         *val = kv.next();

    if( key && strlen(key) ){
      this->parameters[ string(key) ] = val ? strings::urldecode(val) : string();
      log( DEBUG, "    req.params[%s] = '%s'", key, this->parameters[key].c_str() );
    }
  }

  return true;
}

bool http_request::parse_method_and_path(const unsigned char *line, size_t size) {
  string s( (const char *)line );
  std::smatch m;
  if( std::regex_search( s, m, kFirstLineParser ) == false || m.size() != 4 ) {
    log( ERROR, "Error while parsing first line from request: '%s'.", line );
    return false;
  }

  string method = m[1].str();
  if( method == "GET" ){ 
    this->method = GET; 
  }
  else if( method == "POST" ) {
    this->method = POST;
  }
  else if( method == "PATCH" ) {
    this->method = PATCH;
  }
  else if( method == "PUT" ) {
    this->method = PUT;
  }
  else if( method == "CONNECT" ) {
    this->method = CONNECT;
  }
  else {
    log( ERROR, "Invalid HTTP method '%s'", method.c_str() );
    return false;
  }

  this->uri     = m[2].str();
  this->version = m[3].str();

  log( DEBUG, "  req.method  = %s", method.c_str() );
  log( DEBUG, "  req.uri     = %s", this->uri.c_str() );

  s = this->uri;
  if( std::regex_search( s, m, kPathQueryParser ) != false && m.size() == 3 ) {
    this->path = m[1].str();
    string params = m[2].str();
    if( parse_query( params ) == false ){
      return false;
    }
  }
  else {
    this->path = this->uri;
  }

  log( DEBUG, "  req.path    = %s", this->path.c_str() );
  log( DEBUG, "  req.version = %s", this->version.c_str() );

  return true;
}

bool http_request::parse_header(const unsigned char *line, size_t size) {
  string s((const char *)line);
  std::smatch m;
  if( std::regex_search( s, m, kHeaderParser ) == false || m.size() != 3 ) {
    log( ERROR, "Error while parsing header from request: '%s'.", line );
    return false;
  }

  string name  = m[1].str(),
         value = m[2].str();

  log( DEBUG, "  req.headers[%s] = '%s'", name.c_str(), value.c_str() );

  this->headers[name] = value;

  if( name == "Host" ){
    this->host = value;
    log( DEBUG, "    req.host = '%s'", value.c_str() );
  }
  else if( name == "Content-Length" ) {
    this->content_length = atoi( value.c_str() );
    log( DEBUG, "    req.content_length = %d", this->content_length );
  }
  else if( name == "Cookie" ) {
    if( parse_cookies( value ) == false ) {
      // Maybe just log the error and continue?
      return false;
    }
  }

  return true;
}

bool http_request::parse_cookies( const string& s ) {
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

      this->cookies[skey] = strings::urldecode(sval.c_str());
      log( DEBUG, "    req.cookies[%s] = '%s'", skey.c_str(), this->cookies[skey].c_str() );
    }
  }

  return true;
}

bool http_request::parse_line( const unsigned char *line, size_t size ) {
  this->raw += (const char *)line;

  switch( parser_state ) {
    // parse method, path and version
    case PARSE_BEGIN:

      if( parse_method_and_path( line, size ) == true ) {
        log( DEBUG, "PARSE_BEGIN -> PARSE_HEADERS" );
        parser_state = PARSE_HEADERS;
        return true;
      } else {
        log( DEBUG, "PARSE_BEGIN -> PARSE_DONE" );
        parser_state = PARSE_DONE;
        return false;
      }

    break;

    // parse headers
    case PARSE_HEADERS:

      if( strcmp( (char *)line, "\r\n" ) == 0 ) {
        log( DEBUG, "Found end of HTTP headers (conlen=%d needs_body=%s).", content_length, needs_body() ? "yes" : "no" );
        log( DEBUG, "PARSE_HEADERS -> PARSE_DONE" );
        parser_state = PARSE_DONE;
        return true;
      }
      
      if( parse_header( line, size ) == true ) {
        log( DEBUG, "PARSE_HEADERS -> ..." );
        return true;
      } 
      else {
        log( DEBUG, "PARSE_HEADERS -> PARSE_DONE" );
        parser_state = PARSE_DONE;
        return false;
      }

    break;

    // this should never happen
    case PARSE_DONE:
      log( ERROR, "[BUG] http_request::parse_line called after parser state was set to DONE." ); 
    break;
  }

  log( ERROR, "Unhandled http_request for parser state %d: %s", parser_state, line );
  return false;
}

bool http_request::parse_body() {
  log( DEBUG, "Parsing %lu bytes of request body.", this->body.size() );

  if( has_header("Content-Type") ) {
    string content_type = headers["Content-Type"];

    if( content_type == "application/x-www-form-urlencoded" ){
      return parse_query( this->body );  
    } 
    else if( content_type == "application/json" ){
      return parse_json( this->body );
    }
    else {
      log( DEBUG, "Unhandled content type '%s'.", content_type.c_str() );
    }
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

void http_response::text( string text, http_response::Status sts /* = http_response::HTTP_STATUS_OK */ ) {
  status = sts;
  body   = text;
  headers["Content-Type"] = "text/plain";
}

void http_response::html( string html, http_response::Status sts /* = http_response::HTTP_STATUS_OK */ ) {
  status = sts;
  body   = html;
  headers["Content-Type"] = "text/html; charset=utf-8";
}

void http_response::json( string json, http_response::Status sts /* = http_response::HTTP_STATUS_OK */ ) {
  status = sts;
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
