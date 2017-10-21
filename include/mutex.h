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

#include <pthread.h>
 
namespace restd {

class mutex
{ 
  protected:

    pthread_mutex_t _mutex;

  public:

    mutex();
    virtual ~mutex();

    void lock();
    void unlock();

    inline pthread_mutex_t& m() {
      return _mutex;
    }
};

class scoped_mutex {
  private:

    mutex *_mutex;

  public:

    scoped_mutex( mutex *m ) : _mutex(m) {
      _mutex->lock();
    }

    virtual ~scoped_mutex() {
      _mutex->unlock();
    }
};

}

