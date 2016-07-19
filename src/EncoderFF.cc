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

#include <nan.h>
#include "EncoderFF.h"
#include "Memory.h"
#include "EssenceInfo.h"

extern "C" {
  #include <libavutil/opt.h>
  #include <libavcodec/avcodec.h>
  #include <libavutil/imgutils.h>
}

namespace streampunk {

EncoderFF::EncoderFF(std::shared_ptr<EssenceInfo> srcInfo, std::shared_ptr<EssenceInfo> dstInfo, const Duration& duration,
                     uint32_t bitrate, uint32_t gopFrames)
  : mEncoding(dstInfo->encodingName()), mWidth(srcInfo->width()), mHeight(srcInfo->height()), mPixFmt((uint32_t)AV_PIX_FMT_YUV420P),
    mCodec(NULL), mContext(NULL), mFrame(NULL) {

  avcodec_register_all();
  av_log_set_level(AV_LOG_INFO);

  AVCodecID codecID = AV_CODEC_ID_NONE;
  if (!mEncoding.compare("h264"))
    codecID = AV_CODEC_ID_H264;
  else if (!mEncoding.compare("vp8"))
    codecID = AV_CODEC_ID_VP8;

  mCodec = avcodec_find_encoder(codecID);
  if (!mCodec) {
    std::string err = std::string("Encoder for format \'") + mEncoding.c_str() + "\' not found";
    Nan::ThrowError(err.c_str());
    return; 
  }

  mContext = avcodec_alloc_context3(mCodec);
  if (!mContext) {
    Nan::ThrowError("Could not allocate video codec context");
    return; 
  }

  mContext->bit_rate = bitrate;
  //mContext->rc_max_rate = 5000000;
  mContext->width = mWidth;
  mContext->height = mHeight;
  mContext->time_base = { (int)duration.numerator(), (int)duration.denominator() };
  mContext->gop_size = gopFrames;
  mContext->max_b_frames = 1;
  mContext->pix_fmt = (AVPixelFormat)mPixFmt;

  av_opt_set(mContext->priv_data, "slice_mode", "auto", AV_OPT_SEARCH_CHILDREN);
  //av_opt_set(mContext->priv_data, "max_nal_size", "1500", AV_OPT_SEARCH_CHILDREN);
  //av_opt_set(mContext->priv_data, "loopfilter", "1", AV_OPT_SEARCH_CHILDREN);
  av_opt_set(mContext->priv_data, "profile", "baseline", AV_OPT_SEARCH_CHILDREN);
  //av_opt_set(mContext->priv_data, "allow_skip_frames", "true", AV_OPT_SEARCH_CHILDREN);
  //av_opt_set(mContext->priv_data, "cabac", "1", AV_OPT_SEARCH_CHILDREN);

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

EncoderFF::~EncoderFF() {
  av_frame_free(&mFrame);
  avcodec_close(mContext);
  av_free(mContext);
}

uint32_t EncoderFF::bytesReq() const {
  return mWidth * mHeight; // todo: how should this be calculated ??
}

std::string EncoderFF::packingRequired() const {
  return "420P";
}

void EncoderFF::encodeFrame (std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf, 
                             uint32_t frameNum, uint32_t *pDstBytes) {
  // setup source frame data
  mFrame->format = mContext->pix_fmt;
  mFrame->width  = mContext->width;
  mFrame->height = mContext->height;

  mFrame->linesize[0] = mFrame->width;
  mFrame->linesize[1] = mFrame->width / 2;
  mFrame->linesize[2] = mFrame->width / 2;

  uint32_t planeBytes = mContext->width * mContext->height;
  mFrame->data[0] = (uint8_t *)srcBuf->buf();
  mFrame->data[1] = (uint8_t *)(srcBuf->buf() + planeBytes);
  mFrame->data[2] = (uint8_t *)(srcBuf->buf() + planeBytes + planeBytes / 4);

  mFrame->pts = frameNum;
   
  AVPacket pkt;
  av_init_packet(&pkt);
  pkt.data = dstBuf->buf();
  pkt.size = bytesReq();
  int got_output;
  avcodec_encode_video2(mContext, &pkt, mFrame, &got_output);
  *pDstBytes = got_output ? pkt.size : 0;
  
  av_packet_unref(&pkt);
  av_frame_unref(mFrame);
}

} // namespace streampunk
