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
#include "EncodeParams.h"
#include <array>

extern "C" {
  #include <libavutil/opt.h>
  #include <libavcodec/avcodec.h>
  #include <libavutil/imgutils.h>
}

namespace streampunk {

uint32_t getFreqCode(uint32_t sample_rate) {
  uint32_t freqCode = 0;
  std::array<uint32_t, 13> freqCodeArr { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350 };
  while (freqCode < freqCodeArr.size()) {
    if (freqCodeArr[freqCode] == sample_rate)
      break;
    ++freqCode;
  }
  if (freqCodeArr.size() == freqCode)
    throw std::runtime_error("Unsupported sampling frequency for EncoderFF");

  return freqCode;
}

void fillAdtsHeader(uint8_t *buf, uint32_t freqCode, uint32_t chanCode, uint32_t frameSize) {
  buf[0] = 0xFF; // syncword 11..4
  buf[1] = 0xF1; // syncword 3..0 | MPEG Version | Layer | CRC absent
  buf[2] = 0x40 | (freqCode << 2) | (chanCode >> 2); // Audio Object Type 1..0 | Sampling Freq Index 3..0 | private | Channel Config 2 
  buf[3] = (chanCode << 6) | (frameSize >> 11); // Channel Config 1..0 | orig | home | copyrighted id 1..0 | frame length 12..11
  buf[4] = (frameSize & 0x7F8) >> 3;
  buf[5] = ((frameSize & 0x7) << 5) | 0x1F;
  buf[6] = 0xFC;
}


EncoderFF::EncoderFF(std::shared_ptr<EssenceInfo> srcInfo, std::shared_ptr<EssenceInfo> dstInfo, const Duration& duration,
                     std::shared_ptr<EncodeParams> encodeParams)
  : mIsVideo(srcInfo->isVideo()), mEncoding(dstInfo->encodingName()), mBytesReq(0),
    mCodec(NULL), mContext(NULL), mFrame(NULL), mFreqCode(3), mBitsPerSample(16) {

  avcodec_register_all();
  av_log_set_level(AV_LOG_INFO);

  AVCodecID codecID = AV_CODEC_ID_NONE;
  if (mIsVideo) {
    if (!mEncoding.compare("h264"))
      codecID = AV_CODEC_ID_H264;
    else if (!mEncoding.compare("vp8"))
      codecID = AV_CODEC_ID_VP8;
  } else {
    codecID = AV_CODEC_ID_AAC;
  }

  mCodec = avcodec_find_encoder(codecID);
  if (!mCodec) {
    std::string err = std::string("Encoder for format \'") + mEncoding.c_str() + "\' not found";
    Nan::ThrowError(err.c_str());
    return; 
  }

  mContext = avcodec_alloc_context3(mCodec);
  if (!mContext) {
    Nan::ThrowError("Could not allocate av codec context");
    return; 
  }

  if (mIsVideo) {
    mContext->bit_rate = encodeParams->bitrate();
    //mContext->rc_max_rate = 5000000;
    mContext->width = srcInfo->width();
    mContext->height = srcInfo->height();
    mContext->time_base = { (int)duration.numerator(), (int)duration.denominator() };
    mContext->gop_size = encodeParams->gopFrames();
    mContext->max_b_frames = 1;
    mContext->pix_fmt = AV_PIX_FMT_YUV420P;

    av_opt_set(mContext->priv_data, "slice_mode", "auto", AV_OPT_SEARCH_CHILDREN);
    //av_opt_set(mContext->priv_data, "max_nal_size", "1500", AV_OPT_SEARCH_CHILDREN);
    //av_opt_set(mContext->priv_data, "loopfilter", "1", AV_OPT_SEARCH_CHILDREN);
    av_opt_set(mContext->priv_data, "profile", "baseline", AV_OPT_SEARCH_CHILDREN);
    //av_opt_set(mContext->priv_data, "allow_skip_frames", "true", AV_OPT_SEARCH_CHILDREN);
    //av_opt_set(mContext->priv_data, "cabac", "1", AV_OPT_SEARCH_CHILDREN);

    mBytesReq = srcInfo->width() * srcInfo->height(); // todo: how should this be calculated ??
  } else {
    if (srcInfo->encodingName().compare("L16") && 
        srcInfo->encodingName().compare("L20") && 
        srcInfo->encodingName().compare("L24")) {
      Nan::ThrowError("Unsupported audio format");
      return;
    }
    mContext->bit_rate = encodeParams->bitrate();
    mContext->sample_fmt = AV_SAMPLE_FMT_FLTP;
    mContext->sample_rate = srcInfo->clockRate();
    mContext->channels = srcInfo->channels();
    mContext->channel_layout = av_get_default_channel_layout(mContext->channels);
    mContext->profile = FF_PROFILE_AAC_LOW;
    mContext->time_base = { (int)duration.numerator(), (int)duration.denominator() };

    mBytesReq = (uint32_t)mContext->bit_rate / 8; // todo: how should this be calculated ??
    mFreqCode = getFreqCode(mContext->sample_rate);
    mBitsPerSample = std::stoi(srcInfo->encodingName().c_str()+1);
  }

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

std::string EncoderFF::packingRequired() const {
  return "420P";
}

void EncoderFF::encodeFrame (std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf, 
                             uint32_t frameNum, uint32_t *pDstBytes) {
  if (mIsVideo)
    encodeVideo(srcBuf, dstBuf, frameNum, pDstBytes);
  else
    encodeAudio(srcBuf, dstBuf, frameNum, pDstBytes);
}

void EncoderFF::encodeVideo(std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf,
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

void EncoderFF::encodeAudio(std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf,
                            uint32_t frameNum, uint32_t *pDstBytes) {

  mFrame->nb_samples = mContext->frame_size;
  mFrame->format = mContext->sample_fmt;
  mFrame->channel_layout = mContext->channel_layout;
  mFrame->sample_rate = mContext->sample_rate; 

  uint32_t buffer_size = av_samples_get_buffer_size(NULL, mContext->channels, mContext->frame_size, mContext->sample_fmt, 0);
  float *samples = (float *)av_malloc(buffer_size);
  if (avcodec_fill_audio_frame(mFrame, mContext->channels, mContext->sample_fmt,
                               (const uint8_t*)samples, buffer_size, 0) < 0) {
    throw std::runtime_error("EncoderFF could not set up audio frame");
  }

  mFrame->pts = frameNum;

  // convert from S16/S20/S24 format to FLTP
  uint8_t *buf = srcBuf->buf();
  uint32_t numSrcBytes = srcBuf->numBytes();
  uint32_t numSamples = mFrame->nb_samples;
  uint32_t numChannels = mContext->channels;
  uint32_t bytesPerSample = (mBitsPerSample+7)/8;
  uint32_t expectedBytes = numSamples * numChannels * bytesPerSample; 
  std::string str = std::string("srcBytes ") + std::to_string(numSrcBytes) + ", " + 
    std::to_string(expectedBytes) + ", " + std::to_string(numSamples) + ", " + 
    std::to_string(numChannels) + ", " + std::to_string(bytesPerSample) + "\n"; 
  if (numSrcBytes < numSamples * numChannels * bytesPerSample) {
    std::string err = std::string("EncoderFF - insufficient source buffer ") + str;
    throw std::runtime_error(err.c_str());
  }
  else
    printf(str.c_str());

  float maxValue = powf(2.0f, float(mBitsPerSample - 1)) - 1.0f;
  for (uint32_t i = 0; i < numSamples; ++i) {
    for (uint32_t c = 0; c < numChannels; ++c) {
      int32_t sample = 0;
      for (uint32_t b = 0; b < bytesPerSample; ++b) {
        uint8_t ubyte = buf[(i * numChannels + c) * bytesPerSample + b];
        if ((0==b) && (ubyte&0x80)) // sign bit
          sample = 0xFFFFFFFF;
        sample = (sample << 8) | ubyte; // interleaved channels, big endian
      }
      samples[i + c * numSamples] = (float)sample / maxValue; // channel sequential
    }
  }

  uint32_t adtsHeaderSize = 7;

  AVPacket pkt;
  av_init_packet(&pkt);
  pkt.data = dstBuf->buf() + adtsHeaderSize;
  pkt.size = dstBuf->numBytes() - adtsHeaderSize;

  int got_output;
  avcodec_encode_audio2(mContext, &pkt, mFrame, &got_output);
  uint32_t encSize = got_output ? pkt.size : 0;
  uint32_t frameSize = encSize + adtsHeaderSize;

  fillAdtsHeader(dstBuf->buf(), mFreqCode, mContext->channels, frameSize);
  *pDstBytes = frameSize;

  av_freep(&samples);
  av_packet_unref(&pkt);
  av_frame_unref(mFrame);
}

} // namespace streampunk
