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
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "strings.h"
#include "tcp_stream.h"
#include "log.h"
#include "tcp_server.h"

namespace restd {

tcp_server::tcp_server(int port, const char* address) : _lsd(0), _port(port), _address(address), _listening(false) {} 

tcp_server::~tcp_server() {
  log( DEBUG, "Closing tcp_server ..." );
  if(_lsd > 0) {
    close(_lsd);
  }
}

bool tcp_server::start() {
  int result = -1;

  if (_listening == true) {
    log( ERROR, "tcp_server already running." );
    return 0;
  }

  _lsd = socket(PF_INET, SOCK_STREAM, 0);
  struct sockaddr_in address;

  memset(&address, 0, sizeof(address));
  address.sin_family = PF_INET;
  address.sin_port = htons(_port);

  if(_address.size() > 0) {
    result = inet_pton(PF_INET, _address.c_str(), &(address.sin_addr));
    if( result != 1 ){
      log( ERROR, "tcp_server: inet_pton: %d", result );
      return false;
    }
  }
  else {
    address.sin_addr.s_addr = INADDR_ANY;
  }

  int optval = 1;
  setsockopt(_lsd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval); 

  result = bind(_lsd, (struct sockaddr*)&address, sizeof(address));
  if(result != 0) {
    log( ERROR, "tcp_server: bind failed: %d", result );
    return false;
  }

  result = listen(_lsd, 5);
  if(result != 0) {
    log( ERROR, "tcp_server: listen failed: %d", result );
    return false;
  }

  _listening = true;
  return true;
}

tcp_stream *tcp_server::accept() {
  if (_listening == false) {
    log( ERROR, "Called tcp_server::accept before tcp_server::start!" );
    return NULL;
  }

  struct sockaddr_in address;
  socklen_t len = sizeof(address);
  memset(&address, 0, sizeof(address));
  int sd = ::accept(_lsd, (struct sockaddr*)&address, &len);
  if (sd < 0) {
    log( ERROR, "tcp_server::accept failed: %d", sd );
    return NULL;
  }
  return new tcp_stream(sd, &address);
}

void tcp_server::stop() {
  close(_lsd);
  _lsd = -1;
}

}
