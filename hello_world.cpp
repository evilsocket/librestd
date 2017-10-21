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

#include "restd.h"

class hello_world : public restd::http_controller {
  public:
    void handle( restd::http_request& req, restd::http_response& resp ) {
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
  printf( "Usage: %s <-p port> <-D>\n", argvz );
}
int main(int argc, char **argv)
{
  restd::log_level_t llevel = restd::INFO;
  int port = 8080;

  int c;

  while( (c = getopt(argc, argv, "p:Dh")) != -1)
  {
    switch(c)
    {
      case 'p': port   = atoi(optarg); break;
      case 'D': llevel = restd::DEBUG; break;

      default:
          usage(argv[0]);
          return 1;
    }
  }

  try {
    hello_world hw;

    restd::set_log_level( llevel );
    restd::set_log_fp( stdout );
    restd::set_log_dateformat( "%c" );

    restd::crash_manager::init();

    restd::http_server server( "127.0.0.1", port, std::thread::hardware_concurrency() );
    
    server.route( "/hello", &hw );

    server.start();
  }
  catch( const exception& e ) {
    restd::log( restd::CRITICAL, "Exception: %s", e.what() );
  }

  return EXIT_SUCCESS;
}
