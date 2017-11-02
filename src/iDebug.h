/* Copyright 2017 Streampunk Media Ltd.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef IDEBUG_H
#define IDEBUG_H

#include <cstdarg>
#include <stdio.h>

namespace streampunk {

enum eDebugLevel { eFatal = 0, eError, eWarn, eInfo, eDebug, eTrace };

class iDebug {
public:
  iDebug() : mDebugLevel(eInfo) {}
  iDebug(eDebugLevel debugLevel) : mDebugLevel(debugLevel) {}
  virtual ~iDebug() {}

  void setDebug(eDebugLevel level)  { mDebugLevel = level; }
  void printDebug(eDebugLevel level, const char * format, ... ) {
    if (level <= mDebugLevel) {
      va_list argptr;
      va_start(argptr, format);
      vprintf(format, argptr);
      va_end(argptr);
    }
  }
protected:
  eDebugLevel mDebugLevel;
};

} // namespace streampunk

#endif
