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
#include "thread.h"

namespace restd {

static void* runThread(void* arg) {
  return ((thread*)arg)->run();
}

thread::thread() : _tid(0), _running(0), _detached(0) {}

thread::~thread()
{
  if(_running == 1 && _detached == 0) {
    pthread_detach(_tid);
  }
  if(_running == 1) {
    pthread_cancel(_tid);
  }
}

int thread::start()
{
  int result = pthread_create(&_tid, NULL, runThread, this);
  if(result == 0) {
    _running = 1;
  }
  return result;
}

int thread::join()
{
  int result = -1;
  if(_running == 1) {
    result = pthread_join(_tid, NULL);
    if(result == 0) {
      _detached = 0;
    }
  }
  return result;
}

int thread::detach()
{
  int result = -1;
  if(_running == 1 && _detached == 0) {
    result = pthread_detach(_tid);
    if(result == 0) {
      _detached = 1;
    }
  }
  return result;
}

pthread_t thread::self() {
  return _tid;
}

}
