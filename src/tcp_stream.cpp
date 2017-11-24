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
#include "tcp_stream.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace restd {

tcp_stream::tcp_stream(int sd, struct sockaddr_in* address) : _sd(sd) {
  char ip[50] = {0};

  inet_ntop(PF_INET, (struct in_addr*)&(address->sin_addr.s_addr), ip, sizeof(ip)-1);

  _peer_address = ip;
  _peer_port    = ntohs(address->sin_port);
}

tcp_stream::~tcp_stream() {
  close(_sd);
}

ssize_t tcp_stream::send(const unsigned char* buffer, size_t len) {
  return write(_sd, buffer, len);
}

ssize_t tcp_stream::receive(unsigned char* buffer, size_t len, int timeout) {
  if (timeout <= 0) {
    return read(_sd, buffer, len);
  }

  if( wait_readable(timeout) == true ) {
    return read(_sd, buffer, len);
  }

  return TCP_READ_TIMEOUT;
}

ssize_t tcp_stream::read_until(unsigned char until, string& line, int timeout) {
  size_t max = 0xFFFF, wrote;
  unsigned char byte = 0;

  line.clear();

  for( wrote = 0; wrote < max && byte != until; ++wrote ) {
    int r = receive( &byte, 1, timeout );
    if( r != 1 ) {
      return r;
    }

    line += byte;
  }

  return wrote;
}

string tcp_stream::peer_address() {
  return _peer_address;
}

int tcp_stream::peer_port() {
  return _peer_port;
}

bool tcp_stream::wait_readable(int timeout) {
  fd_set sdset;
  struct timeval tv;

  tv.tv_sec = timeout;
  tv.tv_usec = 0;
  FD_ZERO(&sdset);
  FD_SET(_sd, &sdset);
  
  if( select(_sd+1, &sdset, NULL, NULL, &tv) > 0 ) {
    return true;
  }

  return false;
}

}
