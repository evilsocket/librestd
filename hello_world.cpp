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
#include <getopt.h>
#include <thread>
#include <sstream>

#include "restd.h"

class hello_world_controller : public restd::http_controller {
  public:

   // GET /
   void index( restd::http_request& req, restd::http_response& resp ) {
      std::stringstream ss;

      ss << "Welcome to the librestd Hello World, click on one of the following routes:<br><br>";

      ss << "<a href='/hello'>Hello Route</a><br>";
      ss << "<a href='/debug'>Debug Route</a><br>";

      resp.html(ss.str());
   }

   // * /debug
   void debug( restd::http_request& req, restd::http_response& resp ) {
      std::stringstream ss;

      #define KV_LINE(k,v) \
        ss << "<tr><td width='10%' style='font-weight:bold'>" << k << "</td><td>" << v << "</td></tr>"

      #define NESTED(n,v) \
        ss << "<tr><td width='10%' style='font-weight:bold'>" << n << "</td><td>"; \
          ss << "<table width='100%' border='0'>"; \
          for( auto i = (v).begin(), e = (v).end(); i != e; ++i ){ \
            KV_LINE( i->first, i->second ); \
          } \
          ss << "</table>"; \
        ss << "</td></tr>"

      ss << "<strong>THIS IS NOT XSS SAFE</strong><br><br>";

      ss << "<pre>" << req.raw << "</pre>";

      ss << "<table width='100%' border='1'>";

      KV_LINE( "req.method", req.method_name() );
      KV_LINE( "req.uri", req.uri );
      KV_LINE( "req.path", req.path );
      KV_LINE( "req.version", req.version );
      KV_LINE( "req.host", req.host );

      NESTED( "req.headers", req.headers );
      NESTED( "req.cookies", req.cookies );
      NESTED( "req.parameters", req.parameters );

      KV_LINE( "&nbsp;", "&nbsp;" );

      KV_LINE( "req.body", "<pre>" + req.body + "</pre>" );

      ss << "</table>";

      resp.html(ss.str());
    }

   // GET /hello
   void hello( restd::http_request& req, restd::http_response& resp ) {
     string output;

     if( req.has_parameter("name") ){
       output = "Hello world " + req.param("name");
     } else {
       output = "Hello world stranger, use the 'name' parameter to specify your name.";
     }

     resp.html(output);
   }
};

void usage(char *argvz) {
  printf( "Usage: %s <-a address> <-p port> <-D>\n", argvz );
}

int main(int argc, char **argv)
{
  std::string address        = "127.0.0.1";
  int port                   = 8080;
  restd::log_level_t llevel  = restd::INFO;

  int c;

  while( (c = getopt(argc, argv, "a:p:Dh")) != -1)
  {
    switch(c)
    {
      case 'a': address = optarg; break;
      case 'p': port    = atoi(optarg); break;
      case 'D': llevel  = restd::DEBUG; break;

      default:
          usage(argv[0]);
          return 1;
    }
  }

  try {
    hello_world_controller hw;

    restd::set_log_level( llevel );
    restd::set_log_fp( stdout );
    restd::set_log_dateformat( "%c" );

    restd::crash_manager::init();

    restd::http_server server( address.c_str(), port, std::thread::hardware_concurrency() );
    
    server.route( "/",      &hw, (restd::http_controller::handler_t)&hello_world_controller::index, restd::GET );
    server.route( "/debug", &hw, (restd::http_controller::handler_t)&hello_world_controller::debug, restd::ANY );
    server.route( "/hello", &hw, (restd::http_controller::handler_t)&hello_world_controller::hello, restd::GET );

    server.start();
  }
  catch( const exception& e ) {
    restd::log( restd::CRITICAL, "Exception: %s", e.what() );
  }

  return EXIT_SUCCESS;
}
