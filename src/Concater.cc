#include <nan.h>
#include "Concater.h"
#include "MyWorker.h"
#include "Timer.h"
#include "Packers.h"
#include "Memory.h"

#include <memory>

using namespace v8;

namespace streampunk {

class ConcatProcessData : public iProcessData {
public:
  ConcatProcessData (Local<Array> srcBufArray, Local<Object> dstBuf)
    : mDstBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(dstBuf), (uint32_t)node::Buffer::Length(dstBuf))) {
    for (uint32_t i = 0; i < srcBufArray->Length(); ++i) {
      Local<Object> bufferObj = Local<Object>::Cast(srcBufArray->Get(i));
      uint32_t bufLen = (uint32_t)node::Buffer::Length(bufferObj);
      mSrcBufVec.push_back(std::make_pair((uint8_t *)node::Buffer::Data(bufferObj), bufLen)); 
    }
  }
  ~ConcatProcessData() { }
  
  tBufVec srcBufVec() const { return mSrcBufVec; }
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }

private:
  tBufVec mSrcBufVec;
  std::shared_ptr<Memory> mDstBuf;
};


Concater::Concater(std::string format, uint32_t width, uint32_t height) 
  : mFormat(format), mWidth(width), mHeight(height), mWorker(NULL) {

  mDstBytesReq = getFormatBytes(mFormat, mWidth, mHeight);
  if (mWidth % 2) {
    std::string err = std::string("Unsupported width \'") + std::to_string(mWidth) + "\'\n";
    Nan::ThrowError(err.c_str());
  }
}

Concater::~Concater() {}

// iProcess
uint32_t Concater::processFrame (std::shared_ptr<iProcessData> processData) {
  Timer t;
  std::shared_ptr<ConcatProcessData> cpd = std::dynamic_pointer_cast<ConcatProcessData>(processData);

  uint32_t concatBufOffset = 0;
  tBufVec srcBufVec = cpd->srcBufVec();
  for (tBufVec::const_iterator it = srcBufVec.begin(); it != srcBufVec.end(); ++it) {
    const uint8_t* buf = it->first;
    uint32_t len = it->second;
    memcpy (cpd->dstBuf()->buf() + concatBufOffset, buf, len);
    concatBufOffset += len;
  }
  printf("concat : %.2fms\n", t.delta());
  return concatBufOffset;
}

NAN_METHOD(Concater::Start) {
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  Concater* obj = Nan::ObjectWrap::Unwrap<Concater>(info.Holder());

  if (obj->mWorker != NULL)
    Nan::ThrowError("Attempt to restart concater when not idle");
  
  obj->mWorker = new MyWorker(callback);
  AsyncQueueWorker(obj->mWorker);

  info.GetReturnValue().Set(Nan::New(obj->mDstBytesReq));
}

NAN_METHOD(Concater::Concat) {
  Local<Array> srcBufArray = Local<Array>::Cast(info[0]);
  Local<Object> dstBuf = Local<Object>::Cast(info[1]);
  Concater* obj = Nan::ObjectWrap::Unwrap<Concater>(info.Holder());

  if (obj->mWorker == NULL)
    Nan::ThrowError("Attempt to concat when worker not started");
  std::shared_ptr<iProcessData> cpd = std::make_shared<ConcatProcessData>(srcBufArray, dstBuf);
  obj->mWorker->doFrame(cpd, obj, new Nan::Callback(Local<Function>::Cast(info[2])));

  info.GetReturnValue().Set(Nan::New(obj->mWorker->numQueued()));
}

NAN_METHOD(Concater::Quit) {
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  Concater* obj = Nan::ObjectWrap::Unwrap<Concater>(info.Holder());

  if (obj->mWorker != NULL)
    obj->mWorker->quit(callback);

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Concater::Finish) {
  Concater* obj = Nan::ObjectWrap::Unwrap<Concater>(info.Holder());
  delete obj->mWorker;
  obj->mWorker = NULL;
  info.GetReturnValue().SetUndefined();
}

NAN_MODULE_INIT(Concater::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Concater").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(tpl, "start", Start);
  SetPrototypeMethod(tpl, "concat", Concat);
  SetPrototypeMethod(tpl, "quit", Quit);
  SetPrototypeMethod(tpl, "finish", Finish);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("Concater").ToLocalChecked(),
    Nan::GetFunction(tpl).ToLocalChecked());
}

} // namespace streampunk
