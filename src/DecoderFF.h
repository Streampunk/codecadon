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

#ifndef DECODERFF_H
#define DECODERFF_H

#include <memory>
#include "iCodecDriver.h"

struct AVCodec;
struct AVCodecContext;
struct AVFrame;

namespace streampunk {

class Memory;
class EssenceInfo;
class DecoderFF : public iDecoderDriver {
public:
  DecoderFF(std::shared_ptr<EssenceInfo> srcInfo, std::shared_ptr<EssenceInfo> dstInfo);
  ~DecoderFF();

  uint32_t bytesReq() const;
  uint32_t width() const { return mWidth; }
  uint32_t height() const { return mHeight; }
  uint32_t pixFmt() const { return mPixFmt; }
  
  void decodeFrame (std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf, uint32_t frameNum, uint32_t *pDstBytes);

private:
  std::string mSrcEncoding;
  std::string mDstPacking;
  const uint32_t mWidth;
  const uint32_t mHeight;
  const uint32_t mPixFmt;
  AVCodec *mCodec;
  AVCodecContext *mContext;
  AVFrame *mFrame;
};


} // namespace streampunk

#endif
