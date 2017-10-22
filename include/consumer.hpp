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

#include <thread>

namespace restd {

template <typename T> 
class consumer 
{
  protected:

    bool             _running;
    std::thread      _thread;
    work_queue<T *>& _queue;
 
    void run() {
      while(_running) {
        T *item = _queue.remove();
        this->consume(item);
        delete item;
      }
    }

  public:

    consumer(work_queue<T *>& queue) : _queue(queue), _running(false) {}

    void start() {
      _running = true;
      _thread = std::thread(&consumer::run, this);
    }

    void stop() {
      _running = false;
      _thread.join();
    }

    virtual void consume( T *item ) = 0;
};

}
