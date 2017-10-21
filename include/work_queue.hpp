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

#include "mutex.h"
#include <pthread.h>
#include <list>
 
using namespace std;
 
namespace restd {

template <typename T> 
class work_queue
{ 
  protected:

    list<T>    _queue;
    mutex      _mutex;
    condition  _cond;

  public:

    void add(T item) {
      scoped_mutex sm(&_mutex);
      _queue.push_back(item);
      _cond.signal();
    }

    T remove() {
      scoped_mutex sm(&_mutex);

      while( _queue.size() == 0 ) {
        _cond.wait(_mutex);
      }

      T item = _queue.front();
      _queue.pop_front();

      return item;
    }

    int size() {
      scoped_mutex sm(&_mutex);
      int size = _queue.size();
      return size;
    }
};

}
