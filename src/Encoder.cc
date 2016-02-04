#include <nan.h>
#include "Encoder.h"
#include "MyWorker.h"
using namespace v8;

namespace streampunk {

Encoder::Encoder(uint32_t format) : mFormat(format), mWorker(NULL) {
}

Encoder::~Encoder() {
}

// iProcess
uint32_t Encoder::processFrame (tBufVec srcBufVec, char* dstBuf) {
  uint32_t dstBufOffset = 0;
  for (std::vector<std::pair<const char*, uint32_t> >::iterator it = srcBufVec.begin(); it != srcBufVec.end(); ++it) {
    const char* buf = it->first;
    uint32_t len = it->second;
    memcpy (dstBuf + dstBufOffset, buf, len);
    dstBufOffset += len;
  }
  return dstBufOffset;
}

NAN_METHOD(Encoder::Start) {
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  Encoder* obj = Nan::ObjectWrap::Unwrap<Encoder>(info.Holder());

  if (obj->mWorker != NULL)
    Nan::ThrowError("Attempt to restart encoder when not idle");
  
  obj->mWorker = new MyWorker(callback, obj);
  AsyncQueueWorker(obj->mWorker);
}

NAN_METHOD(Encoder::Encode) {
  Local<Array> srcBufArray = Local<Array>::Cast(info[0]);
  Encoder* obj = Nan::ObjectWrap::Unwrap<Encoder>(info.Holder());

  if (obj->mWorker == NULL)
    Nan::ThrowError("Attempt to encode when worker not started");
  obj->mWorker->doFrame(srcBufArray, new Nan::Callback(Local<Function>::Cast(info[1])));

  info.GetReturnValue().SetUndefined();
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
