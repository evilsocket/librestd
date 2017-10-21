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
#include <ctime>

#include <restd.h>

class hello_world : public restd::http_controller {
  public:

   // GET /
   void index( restd::http_request& req, restd::http_response& resp ) {
      std::stringstream ss;

      ss << "Welcome to the librestd Hello World, click on one of the following routes:<br><br>";

      ss << "<a href='/hello'>Hello Route</a><br>";
      ss << "<a href='/json'>JSON Route</a><br>";
      ss << "<a href='/form'>Form Route</a><br>";
      ss << "<a href='/debug'>Debug Route</a><br>";

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

   // GET /json
   void json( restd::http_request& req, restd::http_response& resp ) {
     restd::json j = {
       { "time", std::time(nullptr) },
       { "string", "hello world" },
       { "object", {
           { "foo", "bar" },
         } 
       }
     };

     resp.json( j.dump() );
   }

   // GET /form
   void form( restd::http_request& req, restd::http_response& resp ) {
      std::stringstream ss;

      ss << "Test form to send stuff to /debug route in POST<br><br>";

      ss << "<form action='/debug' method='POST'>";
      ss << " <input type='text' name='some_input' placeholder='Fill me up baby ...'/>";
      ss << " <input type='submit' value='Submit' />";
      ss << "</form>";

      resp.html(ss.str());
   }

   // * /debug
   void debug( restd::http_request& req, restd::http_response& resp ) {
      std::stringstream ss;

      #define KV_LINE(k,v) \
        ss << "<tr><td width='10%' style='font-weight:bold'>" << k << "</td><td><pre>" << v << "</pre></td></tr>"

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

      KV_LINE( "req.body", req.body );

      ss << "</table>";

      resp.html(ss.str());
    }
};

typedef struct {
  std::string        address;
  unsigned short     port;
  restd::log_level_t llevel;
  unsigned int       workers;
}
args_t;

static args_t args = {
  .address = "127.0.0.1",
  .port = 8080,
  .llevel = restd::INFO,
  .workers = std::thread::hardware_concurrency()
};

static struct option long_options[] = {
    { "address", required_argument, NULL, 'a' },
    { "port",    required_argument, NULL, 'p' },
    { "workers", required_argument, NULL, 'w' },
    { "debug",   no_argument,       NULL, 'd' },

    {NULL, 0, NULL, 0}
};

void usage(char *argvz) {
  printf( "Usage: %s <-a|--address ADDRESS> <-p|--port PORT> <-w|--workers N_WORKERS> <-d|--debug>\n", argvz );
}

int main(int argc, char **argv)
{
  int c;

  while( (c = getopt_long(argc, argv, "a:p:w:dh", long_options, NULL)) != -1) {
    switch(c) {
      case 'a': args.address = optarg; break;
      case 'p': args.port    = atoi(optarg); break;
      case 'd': args.llevel  = restd::DEBUG; break;
      case 'w': args.workers = atoi(optarg); break;

      default:
          usage(argv[0]);
          return 1;
    }
  }

  try {
    restd::set_log_level( args.llevel );
    restd::set_log_fp( stdout );
    restd::set_log_dateformat( "%c" );

    restd::crash_manager::init();

    restd::http_server server( args.address.c_str(), args.port, args.workers );
    
    hello_world hw;

    RESTD_ROUTE( server, restd::GET, "/",      hw, hello_world::index );
    RESTD_ROUTE( server, restd::GET, "/hello", hw, hello_world::hello );
    RESTD_ROUTE( server, restd::GET, "/json",  hw, hello_world::json );
    RESTD_ROUTE( server, restd::GET, "/form",  hw, hello_world::form );
    RESTD_ROUTE( server, restd::ANY, "/debug", hw, hello_world::debug );
    
    server.start();
  }
  catch( const exception& e ) {
    restd::log( restd::CRITICAL, "Exception: %s", e.what() );
  }

  return EXIT_SUCCESS;
}
