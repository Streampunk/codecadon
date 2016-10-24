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

#ifndef ENCODERFF_H
#define ENCODERFF_H

#include <memory>
#include "iCodecDriver.h"

struct AVCodec;
struct AVCodecContext;
struct AVFrame;

namespace streampunk {

class Memory;
class Duration;
class EssenceInfo;
class EncodeParams;
class EncoderFF : public iEncoderDriver {
public:
  EncoderFF(std::shared_ptr<EssenceInfo> srcInfo, std::shared_ptr<EssenceInfo> dstInfo, const Duration& duration,
            std::shared_ptr<EncodeParams> encodeParams);
  ~EncoderFF();

  uint32_t bytesReq() const  { return mBytesReq; }
  std::string packingRequired() const;

  void encodeFrame (std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf, uint32_t frameNum, uint32_t *pDstBytes);

private:
  const bool mIsVideo;
  std::string mEncoding; 
  uint32_t mBytesReq;
  AVCodec *mCodec;
  AVCodecContext *mContext;
  AVFrame *mFrame;
  uint32_t mFreqCode;
  uint32_t mBitsPerSample;

  void encodeVideo(std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf, uint32_t frameNum, uint32_t *pDstBytes);
  void encodeAudio(std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf, uint32_t frameNum, uint32_t *pDstBytes);
};


} // namespace streampunk

#endif
