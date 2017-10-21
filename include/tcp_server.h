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

#include "tcp_stream.h"

namespace restd {

class tcp_server 
{
  protected:

    int    _lsd;
    int    _port;
    string _address;
    bool   _listening;

    tcp_server() {}

  public:

    tcp_server(int port, const char* address="");
    ~tcp_server();

    int         start();
    tcp_stream* accept();
    void        stop();
};

}
