#include <nan.h>
#include "Decoder.h"
#include "MyWorker.h"
#include "Timer.h"
#include "Memory.h"
#include "DecoderFF.h"

#include <memory>

using namespace v8;

namespace streampunk {

class DecodeProcessData : public iProcessData {
public:
  DecodeProcessData (Local<Object> srcBuf, const std::string& srcFormat, uint32_t srcWidth, uint32_t srcHeight, Local<Object> dstBuf)
    : mSrcBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(srcBuf), (uint32_t)node::Buffer::Length(srcBuf))),
      mSrcFormat(srcFormat), mSrcWidth(srcWidth), mSrcHeight(srcHeight), 
      mDstBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(dstBuf), (uint32_t)node::Buffer::Length(dstBuf)))
    { }
  ~DecodeProcessData() { }
  
  std::shared_ptr<Memory> srcBuf() const { return mSrcBuf; }
  std::string srcFormat() const { return mSrcFormat; }
  uint32_t srcWidth() const { return mSrcWidth; }
  uint32_t srcHeight() const { return mSrcHeight; }
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }

private:
  std::shared_ptr<Memory> mSrcBuf;
  const std::string mSrcFormat;
  const uint32_t mSrcWidth;
  const uint32_t mSrcHeight;
  std::shared_ptr<Memory> mDstBuf;
};


Decoder::Decoder(std::string format, uint32_t width, uint32_t height) 
  : mFormat(format), mWidth(width), mHeight(height),
    mWorker(NULL), mFrameNum(0) {

  if (mFormat.compare("420P")) {
    std::string err = std::string("Unsupported codec type \'") + mFormat.c_str() + "\'";
    Nan::ThrowError(err.c_str());
    return;
  }
  if (!mWidth || (mWidth % 2) || !mHeight) {
    std::string err = std::string("Unsupported dimensions \'") + std::to_string(mWidth) + "x" + std::to_string(mHeight) + "\'";
    Nan::ThrowError(err.c_str());
    return;
  }
}

Decoder::~Decoder() {}

// iProcess
uint32_t Decoder::processFrame (std::shared_ptr<iProcessData> processData) {
  Timer t;
  std::shared_ptr<DecodeProcessData> dpd = std::dynamic_pointer_cast<DecodeProcessData>(processData);

  // do the decode
  uint32_t dstBytes = 0;
  mDecoder->decodeFrame (dpd->srcFormat(), dpd->srcBuf(), dpd->dstBuf(), mFrameNum++, &dstBytes);
  printf("decode : %.2fms\n", t.delta());

  return dstBytes;
}

NAN_METHOD(Decoder::Start) {
  if (info.Length() != 1)
    return Nan::ThrowError("Decoder start expects 1 argument");
  if (!info[0]->IsFunction())
    return Nan::ThrowError("Decoder start requires a valid callback as the parameter");
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  Decoder* obj = Nan::ObjectWrap::Unwrap<Decoder>(info.Holder());

  if (obj->mWorker != NULL)
    Nan::ThrowError("Attempt to restart decoder when not idle");

  obj->mDecoder = new DecoderFF(obj->mWidth, obj->mHeight);
  obj->mWorker = new MyWorker(callback);
  AsyncQueueWorker(obj->mWorker);

  info.GetReturnValue().Set(Nan::New(obj->mDecoder->bytesReq()));
}

NAN_METHOD(Decoder::Decode) {
  if (info.Length() != 6)
    return Nan::ThrowError("Decoder Decode expects 6 arguments");
  if (!info[0]->IsArray())
    return Nan::ThrowError("Decoder Decode requires a valid source buffer array as the first parameter");
  if (!info[4]->IsObject())
    return Nan::ThrowError("Decoder Decode requires a valid destination buffer as the fifth parameter");
  if (!info[5]->IsFunction())
    return Nan::ThrowError("Decoder Decode requires a valid callback as the sixth parameter");

  Local<Array> srcBufArray = Local<Array>::Cast(info[0]);
  uint32_t srcWidth = Nan::To<uint32_t>(info[1]).FromJust();
  uint32_t srcHeight = Nan::To<uint32_t>(info[2]).FromJust();
  v8::String::Utf8Value srcFmtString(Nan::To<v8::String>(info[3]).ToLocalChecked());
  Local<Object> dstBuf = Local<Object>::Cast(info[4]);
  Local<Function> callback = Local<Function>::Cast(info[5]);

  Decoder* obj = Nan::ObjectWrap::Unwrap<Decoder>(info.Holder());

  if ((srcWidth != obj->mWidth) || (srcHeight != obj->mHeight)) {
    std::string err = std::string("Unsupported dimensions \'") + std::to_string(srcWidth) + "x" + std::to_string(srcHeight) + "\'";
    return Nan::ThrowError(err.c_str());
  }

  std::string srcFmtCode = *srcFmtString;
  if (srcFmtCode.compare("h264") && srcFmtCode.compare("vp8")) {
    std::string err = std::string("Unsupported source format \'") + srcFmtCode.c_str() + "\'";
    return Nan::ThrowError(err.c_str());
  }

  if (1 != srcBufArray->Length()) {
    std::string err = std::string("Decoder requires single source buffer - received ") + std::to_string(srcBufArray->Length());
    return Nan::ThrowError(err.c_str());
  }
  Local<Object> srcBuf = Local<Object>::Cast(srcBufArray->Get(0));

  if (obj->mWorker == NULL)
    Nan::ThrowError("Attempt to decode when worker not started");

  std::shared_ptr<iProcessData> epd = std::make_shared<DecodeProcessData>(srcBuf, srcFmtCode, srcWidth, srcHeight, dstBuf);
  obj->mWorker->doFrame(epd, obj, new Nan::Callback(callback));

  info.GetReturnValue().Set(Nan::New(obj->mWorker->numQueued()));
}

NAN_METHOD(Decoder::Quit) {
  if (info.Length() != 1)
    return Nan::ThrowError("Decoder quit expects 1 argument");
  if (!info[0]->IsFunction())
    return Nan::ThrowError("Decoder quit requires a valid callback as the parameter");
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  Decoder* obj = Nan::ObjectWrap::Unwrap<Decoder>(info.Holder());

  if (obj->mWorker != NULL)
    obj->mWorker->quit(callback);

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Decoder::Finish) {
  Decoder* obj = Nan::ObjectWrap::Unwrap<Decoder>(info.Holder());
  delete obj->mDecoder;
  obj->mWorker = NULL;
  info.GetReturnValue().SetUndefined();
}

NAN_MODULE_INIT(Decoder::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Decoder").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(tpl, "start", Start);
  SetPrototypeMethod(tpl, "decode", Decode);
  SetPrototypeMethod(tpl, "quit", Quit);
  SetPrototypeMethod(tpl, "finish", Finish);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("Decoder").ToLocalChecked(),
    Nan::GetFunction(tpl).ToLocalChecked());
}

} // namespace streampunk
