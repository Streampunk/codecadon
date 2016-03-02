#include <nan.h>
#include "ScaleConverter.h"
#include "MyWorker.h"
#include "Timer.h"
#include "Packers.h"
#include "Memory.h"
#include "ScaleConverterFF.h"

#include <memory>

using namespace v8;

namespace streampunk {

class ScaleConvertProcessData : public iProcessData {
public:
  ScaleConvertProcessData (Local<Object> srcBuf, uint32_t srcWidth, uint32_t srcHeight, std::string srcFmtCode, Local<Object> dstBuf)
    : mSrcBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(srcBuf), (uint32_t)node::Buffer::Length(srcBuf))),
      mSrcWidth(srcWidth), mSrcHeight(srcHeight), 
      mDstBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(dstBuf), (uint32_t)node::Buffer::Length(dstBuf))),
      mPacker(std::make_shared<Packers>(mSrcWidth, mSrcHeight, srcFmtCode, (uint32_t)node::Buffer::Length(srcBuf), "420P"))
    { }
  ~ScaleConvertProcessData() { }
  
  std::shared_ptr<Memory> srcBuf() const { return mSrcBuf; }
  uint32_t srcWidth() const { return mSrcWidth; }
  uint32_t srcHeight() const { return mSrcHeight; }
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }
  std::shared_ptr<Packers> packer() const { return mPacker; }

private:
  std::shared_ptr<Memory> mSrcBuf;
  const uint32_t mSrcWidth;
  const uint32_t mSrcHeight;
  std::shared_ptr<Memory> mDstBuf;
  std::shared_ptr<Packers> mPacker;
};


ScaleConverter::ScaleConverter(std::string format, uint32_t width, uint32_t height) 
  : mFormat(format), mWidth(width), mHeight(height), mWorker(NULL),
    mScaleConverterFF(new ScaleConverterFF) {

  if (mWidth % 2) {
    std::string err = std::string("Unsupported width \'") + std::to_string(mWidth) + "\'\n";
    Nan::ThrowError(err.c_str());
  }

  mDstBytesReq = getFormatBytes(format, width, height);      
}
ScaleConverter::~ScaleConverter() {}

// iProcess
uint32_t ScaleConverter::processFrame (std::shared_ptr<iProcessData> processData) {
  Timer t;
  std::shared_ptr<ScaleConvertProcessData> scpd = std::dynamic_pointer_cast<ScaleConvertProcessData>(processData);

  // format convert to encoder format
  std::shared_ptr<Memory> convertBuf = scpd->packer()->convert(scpd->srcBuf()); 
  printf("convert: %.2fms\n", t.delta());
    
  // scale to encoder size
  std::shared_ptr<Memory> scaledBuf = mScaleConverterFF->scaleConvertFrame (convertBuf, scpd->srcWidth(), scpd->srcHeight(), 0,
                                                                            mWidth, mHeight, 0);
  printf("scale  : %.2fms\n", t.delta());
  return mDstBytesReq;
}

NAN_METHOD(ScaleConverter::Start) {
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  ScaleConverter* obj = Nan::ObjectWrap::Unwrap<ScaleConverter>(info.Holder());

  if (obj->mWorker != NULL)
    Nan::ThrowError("Attempt to restart encoder when not idle");
  
  obj->mWorker = new MyWorker(callback);
  AsyncQueueWorker(obj->mWorker);

  info.GetReturnValue().Set(Nan::New(obj->mDstBytesReq));
}

NAN_METHOD(ScaleConverter::ScaleConvert) {
  Local<Array> srcBufArray = Local<Array>::Cast(info[0]);
  uint32_t srcWidth = Nan::To<uint32_t>(info[1]).FromJust();
  uint32_t srcHeight = Nan::To<uint32_t>(info[2]).FromJust();
  v8::String::Utf8Value srcFmtString(Nan::To<v8::String>(info[3]).ToLocalChecked());
  Local<Object> dstBuf = Local<Object>::Cast(info[4]);
  Local<Function> callback = Local<Function>::Cast(info[5]);

  std::string srcFmtCode = *srcFmtString;
  if (srcFmtCode.compare("4175") && srcFmtCode.compare("v210")) {
    std::string err = std::string("Unsupported source format \'") + srcFmtCode.c_str() + "\'\n";
    return Nan::ThrowError(err.c_str());
  }

  if (1 != srcBufArray->Length()) {
    std::string err = std::string("ScaleConverter requires single source buffer - received ") + std::to_string(srcBufArray->Length()) + "\n";
    return Nan::ThrowError(err.c_str());
  }
  Local<Object> srcBuf = Local<Object>::Cast(srcBufArray->Get(0));

  ScaleConverter* obj = Nan::ObjectWrap::Unwrap<ScaleConverter>(info.Holder());
  if (obj->mWorker == NULL)
    Nan::ThrowError("Attempt to encode when worker not started");

  std::shared_ptr<iProcessData> scpd = 
    std::make_shared<ScaleConvertProcessData>(srcBuf, srcWidth, srcHeight, srcFmtCode, dstBuf);
  obj->mWorker->doFrame(scpd, obj, new Nan::Callback(callback));

  info.GetReturnValue().Set(Nan::New(obj->mWorker->numQueued()));
}

NAN_METHOD(ScaleConverter::Quit) {
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  ScaleConverter* obj = Nan::ObjectWrap::Unwrap<ScaleConverter>(info.Holder());

  if (obj->mWorker != NULL)
    obj->mWorker->quit(callback);

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(ScaleConverter::Finish) {
  ScaleConverter* obj = Nan::ObjectWrap::Unwrap<ScaleConverter>(info.Holder());
  delete obj->mWorker;
  delete obj->mScaleConverterFF;
  obj->mWorker = NULL;
  info.GetReturnValue().SetUndefined();
}

NAN_MODULE_INIT(ScaleConverter::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("ScaleConverter").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(tpl, "start", Start);
  SetPrototypeMethod(tpl, "scaleConvert", ScaleConvert);
  SetPrototypeMethod(tpl, "quit", Quit);
  SetPrototypeMethod(tpl, "finish", Finish);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("ScaleConverter").ToLocalChecked(),
    Nan::GetFunction(tpl).ToLocalChecked());
}

} // namespace streampunk
