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

#include "work_queue.hpp"
#include "consumer.hpp"
#include "tcp_server.h"
#include "http.h"

namespace restd {

class http_controller 
{
  protected:

    static void build_json( http_response& resp, string json, http_response::Status status = http_response::HTTP_STATUS_OK );

  public:
    
    virtual void handle( http_request& req, http_response& resp ) = 0;
};

typedef map<string, http_controller *> routes_t;

class http_consumer : public consumer<tcp_stream> {
  private:

   routes_t *_routes;

    void route( http_request& request, http_response& response );

  public:

    http_consumer(work_queue<tcp_stream *>& queue, routes_t *routes) : consumer(queue), _routes(routes) {}
   
    virtual void consume( tcp_stream *client );
};


class http_server 
{
  private:

   string                    _address;
   unsigned short            _port;
   tcp_server               *_server;
   unsigned int             _threads;
   work_queue<tcp_stream *> _queue;
   list<http_consumer *>    _consumers;
   routes_t                 _routes;

  public:

   http_server( string address, unsigned short port, unsigned int threads );
   virtual ~http_server();

   void route( string path, http_controller *controller );

   void start();
};

}
