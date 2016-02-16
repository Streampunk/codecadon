#include <nan.h>
#include "Encoder.h"
#include "MyWorker.h"

extern "C" {
  #include <libavutil/opt.h>
  #include <libavcodec/avcodec.h>
}

using namespace v8;

namespace streampunk {

class EncodeProcessData : public iProcessData {
public:
  EncodeProcessData (Local<Array> srcBufArray, Local<Object> dstBuf)
    : mSrcBytes(0), mDstBuf(node::Buffer::Data(dstBuf)), mDstBytes(0) {
    mSrcBytes = 0;
    for (uint32_t i = 0; i < srcBufArray->Length(); ++i) {
      Local<Object> bufferObj = Local<Object>::Cast(srcBufArray->Get(i));
      uint32_t bufLen = (uint32_t)node::Buffer::Length(bufferObj);
      mSrcBufVec.push_back(std::make_pair(node::Buffer::Data(bufferObj), bufLen)); 
      mSrcBytes += bufLen;
    }
    mDstBytes = mSrcBytes;
  }

  tBufVec srcBufVec()  { return mSrcBufVec; }
  uint32_t srcBytes()  { return mSrcBytes; }
  char *dstBuf()  { return mDstBuf; }
  uint32_t dstBytes()  { return mDstBytes; }

private:
  tBufVec mSrcBufVec;
  uint32_t mSrcBytes;
  char *mDstBuf;
  uint32_t mDstBytes;
};

void smokeInitOpenH264() {
  avcodec_register_all();
  AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (!codec) {
    fprintf(stderr, "OpenH264 codec not found\n");
    return;
  }

  AVCodecContext *c = avcodec_alloc_context3(codec);
  if (!c) {
    fprintf(stderr, "Could not allocate video codec context\n");
    return;
  }

  /* put sample parameters */
  c->bit_rate = 4000000;
  /* resolution must be a multiple of two */
  c->width = 1920;
  c->height = 1080;
  /* frames per second */
  c->time_base = {1,25};
  /* emit one intra frame every ten frames
   * check frame pict_type before passing frame
   * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
   * then gop_size is ignored and the output of encoder
   * will always be I frame irrespective to gop_size
   */
  c->gop_size = 10;
  c->max_b_frames = 1;
  c->pix_fmt = AV_PIX_FMT_YUV420P;

  av_opt_set(c->priv_data, "profile", "baseline", AV_OPT_SEARCH_CHILDREN);
  av_opt_set(c->priv_data, "level", "3.0", AV_OPT_SEARCH_CHILDREN);
  av_opt_set(c->priv_data, "preset", "slow", AV_OPT_SEARCH_CHILDREN);
  av_opt_set(c->priv_data, "crf",  "18", AV_OPT_SEARCH_CHILDREN);
  av_opt_set(c->priv_data, "allow_skip_frames", false, AV_OPT_SEARCH_CHILDREN);

  /* open it */
  if (avcodec_open2(c, codec, NULL) < 0) {
    fprintf(stderr, "Could not open codec\n");
    return;
  }
}

Encoder::Encoder(uint32_t format) : mFormat(format), mWorker(NULL) {
  smokeInitOpenH264();
}

Encoder::~Encoder() {
}

// iProcess
bool Encoder::processFrame (iProcessData *processData) {
  EncodeProcessData *epd = dynamic_cast<EncodeProcessData *>(processData);
  uint32_t dstBufOffset = 0;
  tBufVec srcBufVec = epd->srcBufVec();
  char *dstBuf = epd->dstBuf();
  for (tBufVec::iterator it = srcBufVec.begin(); it != srcBufVec.end(); ++it) {
    const char* buf = it->first;
    uint32_t len = it->second;
    memcpy (dstBuf + dstBufOffset, buf, len);
    dstBufOffset += len;
  }
  delete epd;
  return true;
}

NAN_METHOD(Encoder::Start) {
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  Encoder* obj = Nan::ObjectWrap::Unwrap<Encoder>(info.Holder());

  if (obj->mWorker != NULL)
    Nan::ThrowError("Attempt to restart encoder when not idle");
  
  obj->mWorker = new MyWorker(callback);
  AsyncQueueWorker(obj->mWorker);
}

NAN_METHOD(Encoder::Encode) {
  Local<Array> srcBufArray = Local<Array>::Cast(info[0]);
  Local<Object> dstBuf = Local<Object>::Cast(info[1]);
  Encoder* obj = Nan::ObjectWrap::Unwrap<Encoder>(info.Holder());

  if (obj->mWorker == NULL)
    Nan::ThrowError("Attempt to encode when worker not started");
  obj->mWorker->doFrame(new EncodeProcessData(srcBufArray, dstBuf), obj, Local<Function>::Cast(info[2]));

  info.GetReturnValue().Set(Nan::New(obj->mWorker->numQueued()));
}

NAN_METHOD(Encoder::Quit) {
  Encoder* obj = Nan::ObjectWrap::Unwrap<Encoder>(info.Holder());

  if (obj->mWorker != NULL) {
    obj->mWorker->quit();
    obj->mWorker = NULL;
  }
  info.GetReturnValue().SetUndefined();
}

NAN_MODULE_INIT(Encoder::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Encoder").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(tpl, "start", Start);
  SetPrototypeMethod(tpl, "encode", Encode);
  SetPrototypeMethod(tpl, "quit", Quit);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("Encoder").ToLocalChecked(),
    Nan::GetFunction(tpl).ToLocalChecked());
}

} // namespace streampunk
