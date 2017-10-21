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

#include <regex>

namespace restd {

class http_controller 
{
  public:
    typedef void (http_controller::*handler_t)( http_request& req, http_response& resp );
};

class http_route 
{
  private:

    bool           is_re;
    std::regex     re;
    vector<string> names;

  public:

    Method               method;
    string               path;
    http_controller     *controller;
    http_controller::handler_t handler;

    http_route( string path, http_controller *controller, http_controller::handler_t handler, Method method = ANY );

    bool matches( http_request& req );
    void call( http_request& req, http_response& resp );
};

typedef list<http_route *> routes_t;

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

   string                   _address;
   unsigned short           _port;
   tcp_server              *_server;
   unsigned int             _threads;
   work_queue<tcp_stream *> _queue;
   list<http_consumer *>    _consumers;
   routes_t                 _routes;

  public:

   http_server( string address, unsigned short port, unsigned int threads );
   virtual ~http_server();

   void route( string path, http_controller *controller, http_controller::handler_t handler, Method method = ANY );

   void start();
};

#define RESTD_ROUTE( SERVER, METHOD, PATH, CONTROLLER, HANDLER ) \
  (SERVER).route( (PATH), &(CONTROLLER), (restd::http_controller::handler_t)&HANDLER, (METHOD) )
}
