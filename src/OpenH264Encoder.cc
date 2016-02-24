#include <nan.h>
#include "OpenH264Encoder.h"
#include "Memory.h"

extern "C" {
  #include <libavutil/opt.h>
  #include <libavcodec/avcodec.h>
  #include <libavutil/imgutils.h>
}

namespace streampunk {

OpenH264Encoder::OpenH264Encoder(uint32_t width, uint32_t height)
  : mWidth(width), mHeight(height), mPixFmt((uint32_t)AV_PIX_FMT_YUV420P),
    mCodec(NULL), mContext(NULL), mFrame(NULL) {

  init();
}

OpenH264Encoder::~OpenH264Encoder() {
  av_frame_free(&mFrame);
  avcodec_close(mContext);
  av_free(mContext);
}

void OpenH264Encoder::init() {
  avcodec_register_all();
  av_log_set_level(AV_LOG_INFO);

  mCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (!mCodec)
    return Nan::ThrowError("OpenH264 codec not found");

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

uint32_t OpenH264Encoder::bytesReq() const {
  return mWidth * mHeight; // todo: how should this be calculated ??
}

void OpenH264Encoder::encodeFrame (std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf, uint32_t frameNum, bool& done) {
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
  pkt.size = planeBytes * 5 / 2;
  int got_output;
  avcodec_encode_video2(mContext, &pkt, mFrame, &got_output);
  done = (got_output != 0);
  
  av_packet_unref(&pkt);
  av_frame_unref(mFrame);
}

} // namespace streampunk
