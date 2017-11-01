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
  unsigned char req_buffer[ http_request::max_size ] = {0},
               *pbuffer = &req_buffer[0];
  int           read = 0,
                left = 0;
  string        res_buffer;
  http_request  request;
  http_response response;
  
  log( DEBUG, "New client connection from %s:%d", client->peer_address().c_str(), client->peer_port() );

  for( left = http_request::max_size; left > 0; ) {
    read = client->receive( pbuffer, left, http_request::read_timeout );
    if( read <= 0 ) {
      if( read == TCP_ERROR ) {
        log( ERROR, "Failed to read request from client: %s", strerror(errno) );
      }
      else if( read == TCP_READ_TIMEOUT ) {
        log( ERROR, "Failed to read request from client: read time out." );
      }
      else {
        log( ERROR, "Failed to read request from client: %d.", read );
      }
      return;
    }

    log( DEBUG, "  Read %d bytes of request from client.", read );

    left    -= read;
    pbuffer += read;

    /*
     * FIXME:
     *
     * If the whole request is sent unbuffered this exit condition
     * might cause the read loop to break before we're done reading
     * the request body. In order to fix the 'Content-Length' header
     * should be compared to the actual amount of bytes read and 
     * further read loops should be executed if something is missing.
     */
    if( strstr( (char *)req_buffer, HTTP_END_OF_HEADERS ) != NULL ) {
      break;
    }
  }

  size_t req_size = http_request::max_size - left;

  log( DEBUG, "Read loop completed, total request size is %lu bytes ( left %d bytes ).", req_size, left );

  // std::regex_error might be thrown
  try {
    if( http_request::parse( request, req_buffer, req_size ) == false ){
      log( ERROR, "Could not parse request: %s", req_buffer );
      response.bad_request();
      goto done;
    }
  }
  catch( const std::regex_error& e ){
    log( ERROR, "Exception (%d) while parsing request: %s", e.code(), e.what() );
    response.bad_request();
    goto done;
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
