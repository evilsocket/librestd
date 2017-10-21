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
#include "http_route.h"
#include "log.h"

namespace restd {

const static std::regex NAMED_PARAM_PARSER( ":([_a-z0-9]+)\\(([^\\)]*)\\)", std::regex_constants::icase );

http_route::http_route( string path, http_controller *controller, http_controller::handler_t handler, Method method /* = ANY */ ) :
  is_re(false),
  method(method),
  path(path),
  controller(controller),
  handler(handler){

  std::smatch m;
  while( std::regex_search( path, m, NAMED_PARAM_PARSER ) == true && m.size() == 3 ) {
    string tok  = m[0].str(),
           name = m[1].str(),
           expr = m[2].str();

    log( DEBUG, "Found named parameter '%s' ( validator='%s' )", name.c_str(), expr.c_str() );

    strings::replace( this->path, tok, "(" + expr + ")" );

    names.push_back(name);

    is_re = true;
    path = m.suffix();
  }

  if(is_re) {
    log( DEBUG, "Named route expression: '%s'", this->path.c_str() );
    re = std::regex( this->path, std::regex_constants::icase );
  }
}

bool http_route::matches( http_request& req ) {
  if( method != ANY && req.method != method ) {
    return false;
  }

  if( is_re == false ) {
    return req.path == path;
  }

  std::smatch m;
  if( std::regex_search( req.path, m, re ) == true && m.size() == names.size() + 1 ) {
    for( int i = 1; i < m.size(); ++i ) {
      req.parameters[ names[i - 1] ] = m[i].str();
    }

    return true;
  }

  return false;
}

void http_route::call( http_request& req, http_response& resp ) {
  (controller->*handler)( req, resp );
}

}
