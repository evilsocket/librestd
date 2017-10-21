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

#include "thread.h"

namespace restd {

template <typename T> 
class consumer : public restd::thread
{
  protected:

    work_queue<T *>& _queue;
 
  public:

    consumer(work_queue<T *>& queue) : _queue(queue) {}

    virtual void consume( T *item ) = 0;

    void* run() {
      while(true) {
        T *item = _queue.remove();
        this->consume(item);
        delete item;
      }

      return NULL;
    }
};

}
