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
#include "http_server.h"
#include "log.h"

namespace restd {

void http_consumer::route( http_request& request, http_response& response ) {
  for( auto i = _routes->begin(), e = _routes->end(); i != e; ++i ){
    http_route *route = *i;

    if( route->matches( request ) ) {
      log( DEBUG, "'%s %s' matched route.", request.method_name().c_str(), request.path.c_str() );
      route->call( request, response );
      return;
    }
  }

  log( WARNING, "No route defined for '%s %s'", request.method_name().c_str(), request.path.c_str() );
  
  response.not_found();
}

void http_consumer::consume( tcp_stream *client ) {
  unsigned char chunk[ http_request::chunk_size ] = {0};
  int           read = 0,
                left = 0,
                toread = 0;
  char          line[0xff] = {0};
  http_request  request;
  http_response response;
  string        res_buffer;
  
  log( DEBUG, "New client connection from %s:%d", client->peer_address().c_str(), client->peer_port() );

#define LOG_FAILED_READ(r) \
    if( r == TCP_ERROR ) { \
      log( ERROR, "Failed to read request from client: %s", strerror(errno) ); \
    } \
    else if( r == TCP_READ_TIMEOUT ) { \
      log( ERROR, "Failed to read request from client: read time out." ); \
    } \
    else { \
      log( ERROR, "Failed to read request from client: %d.", r ); \
    }

  // Read request line by line until the end of headers.
  while( request.parser_state != PARSE_DONE ) {
    int r = client->read_until( (unsigned char)'\n', (unsigned char *)line, 0xff, http_request::read_timeout );
    if( r <= 0 ) {
      LOG_FAILED_READ(r)
      return;
    }

    try {
      if( request.parse_line( (const unsigned char *)line, strlen(line) ) == false ) {
        response.bad_request();
        goto done;
      }
    }
    catch( const std::regex_error& e ){
      log( ERROR, "Exception (%d) while parsing request: %s", e.code(), e.what() );
      response.bad_request();
      goto done;
    }

    if( request.parser_state == PARSE_DONE ) {
      break;
    }
  }

  // If a content-length was set, read the body.
  if( request.needs_body() == true ) {
    log( DEBUG, "Reading %d bytes of request body (in %d bytes chunks).", request.content_length, http_request::chunk_size );

    request.body.reserve( request.content_length );

    for( left = request.content_length; left > 0; ) {
      toread = left < http_request::chunk_size ? left : http_request::chunk_size;

      log( DEBUG, "  Reading chunk of %d bytes.", toread );

      read = client->receive( chunk, toread, http_request::read_timeout );
      if( read <= 0 ) {
        LOG_FAILED_READ(read)
        return;
      }

      left -= read;

      log( DEBUG, "    Read %d/%d bytes of request from client.", request.content_length - left, request.content_length );

      request.body += string( (char *)chunk, read );
    }

    if( request.parse_body() == false ) {
      response.bad_request();
      goto done;
    }
  }

  route( request, response );

done:

  log( INFO, "%s > \"%s %s\" %d %d", 
       client->peer_address().c_str(), 
       request.method_name().c_str(),
       request.path.c_str(),
       response.status,
       response.body.size() );

  res_buffer = response.str();
  int sent = client->send( (unsigned char *)res_buffer.c_str(), res_buffer.size() );
  if( sent != res_buffer.size() ){
    log( ERROR, "Could not send whole response, sent %d out of %lu bytes.", sent, res_buffer.size() );
  }
}

http_server::http_server( string address, unsigned short port, unsigned int threads ) :
   _address(address), _port(port), _threads(threads)
{
  for( unsigned int i = 0; i < threads; ++i ){
    _consumers.push_back( new http_consumer(_queue, &_routes) );
  }

  _server = new tcp_server( port, address.c_str() );
}

http_server::~http_server() {
  log( INFO, "Stopping http_server ..." );

  _server->stop();
  delete _server;

  for( auto i = _consumers.begin(), e = _consumers.end(); i != e; ++i ){
    delete (*i);
  }

  _consumers.clear();

  for( auto i = _routes.begin(), e = _routes.end(); i != e; ++i ){
    delete (*i);
  }

  _routes.clear();
}

void http_server::route( string path, http_controller *controller, http_controller::handler_t handler, unsigned int methods /* = ANY */ ) {
  log( DEBUG, "Registering controller for path '%s'", path.c_str() );
  _routes.push_back( new http_route( path, controller, handler, methods ) );
}

void http_server::start() {
  log( INFO, "Starting http_server ..." );

  if( _server->start() == true ){
    for( auto i = _consumers.begin(), e = _consumers.end(); i != e; ++i ){
      (*i)->start();
    }

    log( INFO, "Server listening on %s:%d with %lu workers ...", _address.c_str(), _port, _threads );
    while(1) {
      tcp_stream *client = _server->accept();
      if( client ){
        _queue.add(client);
      }
    }
  } else {
    log( CRITICAL, "Could not start http_server." );
  }
}

}
