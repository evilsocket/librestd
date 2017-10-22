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

#include <list>
#include <condition_variable>
#include <mutex>
#include <stdio.h>
 
using namespace std;
 
namespace restd {

template <typename T> 
class work_queue
{ 
  protected:

    list<T>            _queue;
    mutex              _mutex;
    condition_variable _avail;

  public:

    void add(T item) {
      std::unique_lock<std::mutex> lock(_mutex);

      _queue.push_back(item);
      _avail.notify_one();
    }

    T remove() {
      std::unique_lock<std::mutex> lock(_mutex);

      while( _queue.size() == 0 ) {
        _avail.wait(lock);
      }

      T item = _queue.front();
      _queue.pop_front();

      return item;
    }

    int size() {
      std::unique_lock<std::mutex> lock(_mutex);

      int size = _queue.size();
      return size;
    }
};

}
