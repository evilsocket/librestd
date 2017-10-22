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
    size_t         re_expected;

  public:

    unsigned int               methods;
    string                     path;
    http_controller           *controller;
    http_controller::handler_t handler;

    http_route( string path, http_controller *controller, http_controller::handler_t handler, unsigned int methods = ANY );

    bool matches( http_request& req );
    void call( http_request& req, http_response& resp );
};

}
