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

extern "C" {
  #include <libavutil/opt.h>
  #include <libavcodec/avcodec.h>
  #include <libavutil/imgutils.h>
}

namespace streampunk {

EncoderFF::EncoderFF(const std::string& format, uint32_t width, uint32_t height)
  : mFormat(format), mWidth(width), mHeight(height), mPixFmt((uint32_t)AV_PIX_FMT_YUV420P),
    mCodec(NULL), mContext(NULL), mFrame(NULL) {

  init();
}

EncoderFF::~EncoderFF() {
  av_frame_free(&mFrame);
  avcodec_close(mContext);
  av_free(mContext);
}

void EncoderFF::init() {
  avcodec_register_all();
  av_log_set_level(AV_LOG_INFO);

  AVCodecID codecID = AV_CODEC_ID_NONE;
  if (!mFormat.compare("h264"))
    codecID = AV_CODEC_ID_H264;
  else if (!mFormat.compare("vp8"))
    codecID = AV_CODEC_ID_VP8;

  mCodec = avcodec_find_encoder(codecID);
  if (!mCodec) {
    std::string err = std::string("Encoder for format \'") + mFormat.c_str() + "\' not found";
    return Nan::ThrowError(err.c_str());
  }

  mContext = avcodec_alloc_context3(mCodec);
  if (!mContext)
    return Nan::ThrowError("Could not allocate video codec context");

  mContext->bit_rate = 4000000;
  mContext->rc_max_rate = 5000000;
  mContext->width = mWidth;
  mContext->height = mHeight;
  mContext->time_base = {1,25};
  mContext->gop_size = 10;
  mContext->max_b_frames = 1;
  mContext->pix_fmt = (AVPixelFormat)mPixFmt;

  av_opt_set(mContext->priv_data, "slice_mode", "auto", AV_OPT_SEARCH_CHILDREN);
  //av_opt_set(mContext->priv_data, "max_nal_size", "1500", AV_OPT_SEARCH_CHILDREN);
  //av_opt_set(mContext->priv_data, "loopfilter", "1", AV_OPT_SEARCH_CHILDREN);
  av_opt_set(mContext->priv_data, "profile", "baseline", AV_OPT_SEARCH_CHILDREN);
  //av_opt_set(mContext->priv_data, "allow_skip_frames", "true", AV_OPT_SEARCH_CHILDREN);
  //av_opt_set(mContext->priv_data, "cabac", "1", AV_OPT_SEARCH_CHILDREN);

  if (avcodec_open2(mContext, mCodec, NULL) < 0)
    return Nan::ThrowError("Could not open codec");

  mFrame = av_frame_alloc();
  if (!mFrame)
    return Nan::ThrowError("Could not allocate video frame");
}

uint32_t EncoderFF::bytesReq() const {
  return mWidth * mHeight; // todo: how should this be calculated ??
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
