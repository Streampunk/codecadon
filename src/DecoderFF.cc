#include <nan.h>
#include "DecoderFF.h"
#include "Memory.h"
#include "Packers.h"

extern "C" {
  #include <libavutil/opt.h>
  #include <libavcodec/avcodec.h>
  #include <libavutil/imgutils.h>
}

namespace streampunk {

DecoderFF::DecoderFF(uint32_t width, uint32_t height)
  : mWidth(width), mHeight(height), mPixFmt((uint32_t)AV_PIX_FMT_YUV420P),
    mCodec(NULL), mContext(NULL), mFrame(NULL) {

  init();
}

DecoderFF::~DecoderFF() {
  av_parser_close(mParserContext);
  av_frame_free(&mFrame);
  avcodec_close(mContext);
  av_free(mContext);
}

void DecoderFF::init() {
  avcodec_register_all();
  av_log_set_level(AV_LOG_INFO);

  mCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
  if (!mCodec)
    return Nan::ThrowError("h264 decoder not found");

  mContext = avcodec_alloc_context3(mCodec);
  if (!mContext)
    return Nan::ThrowError("Could not allocate video codec context");

  mContext->width = mWidth;
  mContext->height = mHeight;

  if (avcodec_open2(mContext, mCodec, NULL) < 0)
    return Nan::ThrowError("Could not open codec");

  mFrame = av_frame_alloc();
  if (!mFrame)
    return Nan::ThrowError("Could not allocate video frame");

  mParserContext = av_parser_init(AV_CODEC_ID_H264);
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
