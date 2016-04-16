/* Copyright 2016 Christine S. MacNeill

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

#ifndef PACKERS_H
#define PACKERS_H

#include <memory>
#include "iProcess.h"

namespace streampunk {

class Memory;
class Packers {
public:
  Packers(uint32_t srcWidth, uint32_t srcHeight, const std::string& srcFmtCode, uint32_t srcBytes, const std::string& dstFmtCode);

  void convert(std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf);

private:  
  void convertPGroupto420P (const uint8_t *const srcBuf, uint8_t *const dstBuf);
  void convertV210to420P (const uint8_t *const srcBuf, uint8_t *const dstBuf);

  uint32_t mSrcWidth;
  uint32_t mSrcHeight;
  std::string mSrcFmtCode;
  uint32_t mSrcBytes;
  std::string mDstFmtCode;
};

uint32_t getFormatBytes(const std::string& fmtCode, uint32_t width, uint32_t height);
void dumpPGroupRaw (uint8_t *pgbuf, uint32_t width, uint32_t numLines);
void dump420P (uint8_t *buf, uint32_t width, uint32_t height, uint32_t numLines);

} // namespace streampunk

#endif
