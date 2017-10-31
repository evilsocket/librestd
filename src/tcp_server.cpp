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

#include <sys/un.h>
#include <sys/stat.h>

namespace restd {

tcp_server::tcp_server(int port, const char* address) : _lsd(0), _port(port), _address(address), _listening(false), _is_unix(false), _domain(AF_INET) {
  _is_unix = address[0] == '/';
  _domain  = _is_unix ? AF_LOCAL : AF_INET;
} 

tcp_server::~tcp_server() {
  log( DEBUG, "Closing tcp_server ..." );
  if(_lsd > 0) {
    close(_lsd);
  }
}

bool tcp_server::start() {
  int result = -1;
  struct sockaddr *paddress = NULL;
  size_t addrsz = 0;

  if (_listening == true) {
    log( ERROR, "tcp_server already running." );
    return 0;
  }

  // LSD ftw!
  _lsd = socket( _domain, SOCK_STREAM, 0 );
  if( _lsd == -1 ) {
    log( ERROR, "tcp_server: main socket creation failed: %s", strerror(errno) );
    return false;
  }

  // Make sure connection-intensive things will be able to close/open sockets a zillion of times.
  int optval = 1;
  if( setsockopt(_lsd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) == -1 ) {
    log( ERROR, "tcp_server: setsockopt( SO_REUSEADDR ) failed: %s", strerror(errno) );
    return false;
  }

  // bind to unix socket, _port parameter contains permissions.
  if( _is_unix ) {
    log( DEBUG, "Creating UNIX socket on '%s', mode is %u.", _address.c_str(), _port );

    struct sockaddr_un unix_address;

    paddress = (struct sockaddr *)&unix_address;
    addrsz   = sizeof(unix_address);

    memset(&unix_address, 0, addrsz);

    unix_address.sun_family = AF_LOCAL;
    strncpy( unix_address.sun_path, _address.c_str(), sizeof(unix_address.sun_path) - 1 );
  } 
  // bind to ip socket.
  else {
    log( DEBUG, "Creating IP socket on '%s', port is %u.", _address.c_str(), _port );

    struct sockaddr_in ip_address;

    paddress = (struct sockaddr *)&ip_address;
    addrsz   = sizeof(ip_address);

    memset(&ip_address, 0, addrsz);
    ip_address.sin_family = PF_INET;
    ip_address.sin_port = htons(_port);

    if(_address.size() > 0) {
      result = inet_pton(PF_INET, _address.c_str(), &(ip_address.sin_addr));
      if( result != 1 ){
        log( ERROR, "tcp_server: inet_pton: %s", strerror(errno) );
        return false;
      }
    }
    else {
      ip_address.sin_addr.s_addr = INADDR_ANY;
    }
  }

  result = bind(_lsd, paddress, addrsz);
  if(result != 0) {
    log( ERROR, "tcp_server: bind failed: %s", strerror(errno) );
    return false;
  }

  result = listen(_lsd, 5);
  if(result != 0) {
    log( ERROR, "tcp_server: listen failed: %s", strerror(errno) );
    return false;
  }

  if( _is_unix ) {
    if( chmod( _address.c_str(), (mode_t)_port ) != 0 ){
      log( ERROR, "tcp_server: chmod( %s, %u ) failed: %s", _address.c_str(), _port, strerror(errno) );
      return false;
    }
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
    log( ERROR, "tcp_server::accept failed: %s", strerror(errno) );
    return NULL;
  }
  return new tcp_stream(sd, &address);
}

void tcp_server::stop() {
  close(_lsd);
  _lsd = -1;
}

}
