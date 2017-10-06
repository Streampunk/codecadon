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

#ifndef PACKERS_H
#define PACKERS_H

#include <memory>
#include <functional>
#include "iProcess.h"

namespace streampunk {

class Memory;
class Packers {
public:
  Packers(uint32_t srcWidth, uint32_t srcHeight, const std::string& srcFmtCode, const std::string& dstFmtCode);

  void convert(std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf) const;

private:
  typedef std::function<void(const Packers&, const uint8_t *const, uint8_t *const)> tConvertFn;
  void convertNotSupported (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {}

  void convertPGrouptoUYVY10 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;
  void convertYUV422P10toUYVY10 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;
  void convertPGrouptoYUV422P10 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;
  void convertV210toYUV422P10 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;
  void convertPGroupto420P (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;
  void convertV210to420P (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;

  void convertUYVY10toPGroup (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;
  void convertUYVY10toYUV422P10 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;
  void convertUYVY10to420P (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;
  void convertYUV422P10to420P (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;
  void convertYUV422P10toPGroup (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;
  void convert420PtoPGroup (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;
  void convertYUV422P10toV210 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;
  void convert420PtoV210 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;

  void convertPGrouptoV210 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;
  void convertV210toPGroup (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;

  void convertBGR10AtoGBRP16 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const;

  const uint32_t mSrcWidth;
  const uint32_t mSrcHeight;
  const std::string mSrcFmtCode;
  const std::string mDstFmtCode;
  mutable tConvertFn mConvertFn;
};

uint32_t getFormatBytes(const std::string& fmtCode, uint32_t width, uint32_t height, bool hasAlpha = false);
void dumpPGroupRaw (const uint8_t *const pgbuf, uint32_t width, uint32_t numLines);
void dump420P (const uint8_t *const buf, uint32_t width, uint32_t height, uint32_t numLines);

} // namespace streampunk

#endif
