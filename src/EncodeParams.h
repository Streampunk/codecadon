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

#ifndef ENCODEPARAMS_H
#define ENCODEPARAMS_H

#include <nan.h>
#include <sstream>
#include "Params.h"

using namespace v8;

namespace streampunk {

class EncodeParams : public Params {
public:
  EncodeParams(Local<Object> tags, bool isVideo)
    : Params(isVideo),
      mBitrate(unpackNum(tags, "bitrate", mIsVideo?5000000:128000)),
      mGopFrames(unpackNum(tags, "gopFrames", mIsVideo?90:0))
  {}
  ~EncodeParams() {}

  uint32_t bitrate() const  { return mBitrate; }
  uint32_t gopFrames() const  { return mGopFrames; }

  std::string toString() const  { 
    std::stringstream ss;
    if (mIsVideo)
      ss << "Video encode, bitrate " << mBitrate << ", GOP frames " << mGopFrames;
    else 
      ss << "Audio encode, bitrate " << mBitrate;
    return ss.str();
  }

private:
  uint32_t mBitrate;
  uint32_t mGopFrames;
};

} // namespace streampunk

#endif
