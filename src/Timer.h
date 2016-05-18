/* Copyright 2016 Streampunk Media Ltd.

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

#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <ctime>

namespace streampunk {

class Timer {
public:
  Timer() {
    mStart = std::chrono::high_resolution_clock::now();
    mLast = mStart;
  }

  float delta() {
    auto cur = std::chrono::high_resolution_clock::now();
    float result = (float)std::chrono::duration_cast<std::chrono::microseconds>(cur - mLast).count() / 1000.0f; 
    mLast = cur;
    return result; 
  }

  float total() {
    auto cur = std::chrono::high_resolution_clock::now();
    return (float)std::chrono::duration_cast<std::chrono::microseconds>(cur - mStart).count() / 1000.0f; 
  }

private:
  std::chrono::time_point<std::chrono::high_resolution_clock> mStart;
  std::chrono::time_point<std::chrono::high_resolution_clock> mLast;
};

} // namespace streampunk

#endif