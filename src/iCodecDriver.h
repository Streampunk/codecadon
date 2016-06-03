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

#ifndef CODECDRIVER_H
#define CODECDRIVER_H

#include <memory>

namespace streampunk {

class Memory;

class iEncoderDriver {
public:
  virtual ~iEncoderDriver() {}

  virtual uint32_t bytesReq() const = 0;
  virtual std::string packingRequired() const = 0;
  virtual void encodeFrame (std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf, uint32_t frameNum, uint32_t *pDstBytes) = 0;
};

class iDecoderDriver {
public:
  virtual ~iDecoderDriver() {}

  virtual uint32_t bytesReq() const = 0;
  virtual void decodeFrame (std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf, uint32_t frameNum, uint32_t *pDstBytes) = 0;
};

} // namespace streampunk

#endif