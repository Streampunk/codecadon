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

#include <nan.h>
#include "DecoderFF.h"
#include "Memory.h"
#include "Packers.h"
#include "EssenceInfo.h"

extern "C" {
  #include <libavutil/opt.h>
  #include <libavcodec/avcodec.h>
  #include <libavutil/imgutils.h>
}

namespace streampunk {

DecoderFF::DecoderFF(std::shared_ptr<EssenceInfo> srcInfo, std::shared_ptr<EssenceInfo> dstInfo)
  : mSrcEncoding(srcInfo->encodingName()), mDstPacking(dstInfo->packing()), mWidth(srcInfo->width()), mHeight(srcInfo->height()),
    mPixFmt((uint32_t)AV_PIX_FMT_YUV420P), mCodec(NULL), mContext(NULL), mFrame(NULL) {

  avcodec_register_all();
  av_log_set_level(AV_LOG_INFO);

  AVCodecID codecID = AV_CODEC_ID_NONE;
  if (!mSrcEncoding.compare("h264"))
    codecID = AV_CODEC_ID_H264;
  else if (!mSrcEncoding.compare("vp8"))
    codecID = AV_CODEC_ID_VP8;

  mCodec = avcodec_find_decoder(codecID);
  if (!mCodec) {
    std::string err = std::string("Decoder for format \'") + mSrcEncoding.c_str() + "\' not found";
    Nan::ThrowError(err.c_str());
    return;
  }

  mContext = avcodec_alloc_context3(mCodec);
  if (!mContext) {
    Nan::ThrowError("Could not allocate video codec context");
    return;
  }

  mContext->width = mWidth;
  mContext->height = mHeight;
  mContext->refcounted_frames = 1;

  if (avcodec_open2(mContext, mCodec, NULL) < 0) {
    Nan::ThrowError("Could not open codec");
    return;
  }

  mFrame = av_frame_alloc();
  if (!mFrame) {
    Nan::ThrowError("Could not allocate video frame");
    return;
  }
}

DecoderFF::~DecoderFF() {
  av_frame_free(&mFrame);
  avcodec_close(mContext);
  av_free(mContext);
}

uint32_t DecoderFF::bytesReq() const {
  return mWidth * mHeight * 3 / 2;
}

void DecoderFF::decodeFrame (std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf, 
                             uint32_t frameNum, uint32_t *pDstBytes) {
  AVPacket pkt;
  av_init_packet(&pkt);
  pkt.data = srcBuf->buf();
  pkt.size = srcBuf->numBytes();
  int got_output;
  int bytesUsed = avcodec_decode_video2(mContext, mFrame, &got_output, &pkt);

  if (bytesUsed <= 0) {
    printf("Failed to decode\n");
    *pDstBytes = 0;
  }
  else {
    uint32_t lumaBytes = mFrame->width * mFrame->height;
    uint32_t chromaBytes = lumaBytes / 4;
    memcpy(dstBuf->buf(), mFrame->data[0], lumaBytes);
    memcpy(dstBuf->buf() + lumaBytes, mFrame->data[1], chromaBytes);
    memcpy(dstBuf->buf() + lumaBytes + chromaBytes, mFrame->data[2], chromaBytes);
    *pDstBytes = lumaBytes + chromaBytes * 2;
  }
  
  av_packet_unref(&pkt);
  av_frame_unref(mFrame);
}

} // namespace streampunk
