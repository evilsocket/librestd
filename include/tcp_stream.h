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

#include <unistd.h>
#include <netinet/in.h>
#include <string>

using std::string;

namespace restd {

#define TCP_ERROR        -1
#define TCP_READ_TIMEOUT -2

class tcp_stream
{
  protected:

    int     _sd;
    string  _peer_address;
    int     _peer_port;

    bool wait_readable(int timeout);

  public:

    tcp_stream(int sd, struct sockaddr_in* address);
    tcp_stream();
    tcp_stream(const tcp_stream& stream);
    ~tcp_stream();

    ssize_t send(const unsigned char* buffer, size_t len);
    ssize_t receive(unsigned char* buffer, size_t len, int timeout=0);

    string peer_address();
    int    peer_port();
};

}
