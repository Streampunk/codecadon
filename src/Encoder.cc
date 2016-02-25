#include <nan.h>
#include "Encoder.h"
#include "MyWorker.h"
#include "Timer.h"
#include "Packers.h"
#include "Memory.h"
#include "ScaleConverter.h"
#include "OpenH264Encoder.h"

#include <memory>

using namespace v8;

namespace streampunk {

class EncodeProcessData : public iProcessData {
public:
  EncodeProcessData (Local<Array> srcBufArray, uint32_t srcWidth, uint32_t srcHeight, std::string srcFmtCode, Local<Object> dstBuf)
    : mSrcWidth(srcWidth), mSrcHeight(srcHeight), 
      mDstBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(dstBuf), (uint32_t)node::Buffer::Length(dstBuf))) {
    uint32_t srcBytes = 0;
    for (uint32_t i = 0; i < srcBufArray->Length(); ++i) {
      Local<Object> bufferObj = Local<Object>::Cast(srcBufArray->Get(i));
      uint32_t bufLen = (uint32_t)node::Buffer::Length(bufferObj);
      mSrcBufVec.push_back(std::make_pair((uint8_t *)node::Buffer::Data(bufferObj), bufLen)); 
      srcBytes += bufLen;
    }
    mPacker = std::make_shared<Packers>(mSrcWidth, mSrcHeight, srcFmtCode, srcBytes, "420P");
  }
  ~EncodeProcessData() { }
  
  tBufVec srcBufVec() const { return mSrcBufVec; }
  uint32_t srcWidth() const { return mSrcWidth; }
  uint32_t srcHeight() const { return mSrcHeight; }
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }
  std::shared_ptr<Packers> packer() const { return mPacker; }

private:
  tBufVec mSrcBufVec;
  const uint32_t mSrcWidth;
  const uint32_t mSrcHeight;
  std::shared_ptr<Memory> mDstBuf;
  std::shared_ptr<Packers> mPacker;
};


Encoder::Encoder(std::string format, uint32_t width, uint32_t height) 
  : mFormat(format), mWidth(width), mHeight(height),
    mWorker(NULL), mFrameNum(0) {
  if (mFormat.compare("h264")) {
    std::string err = std::string("Unsupported codec type \'") + mFormat.c_str() + "\'\n";
    Nan::ThrowError(err.c_str());
  }
  if (mWidth % 2) {
    std::string err = std::string("Unsupported width \'") + std::to_string(mWidth) + "\'\n";
    Nan::ThrowError(err.c_str());
  }
}

Encoder::~Encoder() {}

// iProcess
uint32_t Encoder::processFrame (std::shared_ptr<iProcessData> processData) {
  Timer t;
  std::shared_ptr<EncodeProcessData> epd = std::dynamic_pointer_cast<EncodeProcessData>(processData);

  // concatenate source buffers
  std::shared_ptr<Memory> concatBuf = epd->packer()->concatBuffers(epd->srcBufVec());
  printf("concat : %.2fms\n", t.delta());

  // format convert to encoder format
  std::shared_ptr<Memory> convertBuf = epd->packer()->convert(concatBuf); 
  printf("convert: %.2fms\n", t.delta());
    
  // scale to encoder size
  std::shared_ptr<Memory> scaledBuf = mScaleConverter->scaleConvertFrame (convertBuf, epd->srcWidth(), epd->srcHeight(), 0,
                                                                          mEncoder->width(), mEncoder->height(), mEncoder->pixFmt());
  printf("scale  : %.2fms\n", t.delta());
  
  // do the encode
  uint32_t dstBytes = 0;
  mEncoder->encodeFrame (convertBuf, epd->dstBuf(), mFrameNum++, &dstBytes);
  printf("encode : %.2fms\n", t.delta());
  printf("total  : %.2fms\n", t.total());

  return dstBytes;
}

NAN_METHOD(Encoder::Start) {
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  Encoder* obj = Nan::ObjectWrap::Unwrap<Encoder>(info.Holder());

  if (obj->mWorker != NULL)
    Nan::ThrowError("Attempt to restart encoder when not idle");
  
  obj->mWorker = new MyWorker(callback);
  AsyncQueueWorker(obj->mWorker);

  obj->mEncoder = new OpenH264Encoder(obj->mWidth, obj->mHeight);
  obj->mScaleConverter = new ScaleConverter();

  info.GetReturnValue().Set(Nan::New(obj->mEncoder->bytesReq()));
}

NAN_METHOD(Encoder::Encode) {
  Local<Array> srcBufArray = Local<Array>::Cast(info[0]);
  uint32_t srcWidth = Nan::To<uint32_t>(info[1]).FromJust();
  uint32_t srcHeight = Nan::To<uint32_t>(info[2]).FromJust();

  v8::String::Utf8Value srcFmtString(Nan::To<v8::String>(info[3]).ToLocalChecked());
  std::string srcFmtCode = *srcFmtString;
  if (srcFmtCode.compare("4175") && srcFmtCode.compare("v210")) {
    std::string err = std::string("Unsupported source format \'") + srcFmtCode.c_str() + "\'\n";
    return Nan::ThrowError(err.c_str());
  }

  Local<Object> dstBuf = Local<Object>::Cast(info[4]);
  Encoder* obj = Nan::ObjectWrap::Unwrap<Encoder>(info.Holder());

  if (obj->mWorker == NULL)
    Nan::ThrowError("Attempt to encode when worker not started");
  std::shared_ptr<iProcessData> epd = std::make_shared<EncodeProcessData>(srcBufArray, srcWidth, srcHeight, srcFmtCode, dstBuf);
  obj->mWorker->doFrame(epd, obj, new Nan::Callback(Local<Function>::Cast(info[5])));

  info.GetReturnValue().Set(Nan::New(obj->mWorker->numQueued()));
}

NAN_METHOD(Encoder::Quit) {
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  Encoder* obj = Nan::ObjectWrap::Unwrap<Encoder>(info.Holder());

  if (obj->mWorker != NULL)
    obj->mWorker->quit(callback);

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Encoder::Finish) {
  Encoder* obj = Nan::ObjectWrap::Unwrap<Encoder>(info.Holder());
  delete obj->mWorker;
  delete obj->mEncoder;
  delete obj->mScaleConverter;
  obj->mWorker = NULL;
  info.GetReturnValue().SetUndefined();
}

NAN_MODULE_INIT(Encoder::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Encoder").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(tpl, "start", Start);
  SetPrototypeMethod(tpl, "encode", Encode);
  SetPrototypeMethod(tpl, "quit", Quit);
  SetPrototypeMethod(tpl, "finish", Finish);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("Encoder").ToLocalChecked(),
    Nan::GetFunction(tpl).ToLocalChecked());
}

} // namespace streampunk
