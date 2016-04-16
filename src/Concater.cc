/* Copyright 2016 Christine S. MacNeill

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
    : mDstBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(dstBuf), (uint32_t)node::Buffer::Length(dstBuf))), mSrcBytes(0) {
    for (uint32_t i = 0; i < srcBufArray->Length(); ++i) {
      Local<Object> bufferObj = Local<Object>::Cast(srcBufArray->Get(i));
      uint32_t bufLen = (uint32_t)node::Buffer::Length(bufferObj);
      mSrcBufVec.push_back(std::make_pair((uint8_t *)node::Buffer::Data(bufferObj), bufLen)); 
      mSrcBytes += bufLen;
    }
  }
  ~ConcatProcessData() {}
  
  tBufVec srcBufVec() const { return mSrcBufVec; }
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }
  uint32_t srcBytes() const { return mSrcBytes; }

private:
  tBufVec mSrcBufVec;
  std::shared_ptr<Memory> mDstBuf;
  uint32_t mSrcBytes;
};


Concater::Concater(uint32_t numBytes) : mNumBytes(numBytes), mWorker(NULL) {}
Concater::~Concater() {}

// iProcess
uint32_t Concater::processFrame (std::shared_ptr<iProcessData> processData) {
  Timer t;
  std::shared_ptr<ConcatProcessData> cpd = std::dynamic_pointer_cast<ConcatProcessData>(processData);

  uint32_t concatBufOffset = 0;
  tBufVec srcBufVec = cpd->srcBufVec();
  std::shared_ptr<Memory> dstBuf = cpd->dstBuf();
  for (tBufVec::const_iterator it = srcBufVec.begin(); it != srcBufVec.end(); ++it) {
    const uint8_t* srcBuf = it->first;

    uint32_t len = it->second;
    if (concatBufOffset + len > dstBuf->numBytes())
      len = dstBuf->numBytes() - concatBufOffset;

    memcpy (dstBuf->buf() + concatBufOffset, srcBuf, len);
    concatBufOffset += len;
  }
  printf("concat : %.2fms\n", t.delta());
  return concatBufOffset;
}

NAN_METHOD(Concater::Start) {
  if (info.Length() != 1)
    return Nan::ThrowError("Concater start expects 1 argument");
  if (!info[0]->IsFunction())
    return Nan::ThrowError("Concater start requires a valid callback as the parameter");
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  Concater* obj = Nan::ObjectWrap::Unwrap<Concater>(info.Holder());

  if (obj->mWorker != NULL)
    return Nan::ThrowError("Attempt to restart concater when not idle");
  
  obj->mWorker = new MyWorker(callback);
  AsyncQueueWorker(obj->mWorker);

  info.GetReturnValue().Set(Nan::New(obj->mNumBytes));
}

NAN_METHOD(Concater::Concat) {
  if (info.Length() != 3)
    return Nan::ThrowError("Concater concat expects 3 arguments");
  if (!info[0]->IsArray())
    return Nan::ThrowError("Concater concat requires a valid source buffer array as the first parameter");
  if (!info[1]->IsObject())
    return Nan::ThrowError("Concater concat requires a valid destination buffer as the second parameter");
  if (!info[2]->IsFunction())
    return Nan::ThrowError("Concater concat requires a valid callback as the third parameter");
  Local<Array> srcBufArray = Local<Array>::Cast(info[0]);
  Local<Object> dstBuf = Local<Object>::Cast(info[1]);
  Local<Function> callback = Local<Function>::Cast(info[2]);

  Concater* obj = Nan::ObjectWrap::Unwrap<Concater>(info.Holder());

  if (obj->mWorker == NULL)
    return Nan::ThrowError("Attempt to concat when worker not started");

  std::shared_ptr<ConcatProcessData> cpd = std::make_shared<ConcatProcessData>(srcBufArray, dstBuf);
  if (cpd->srcBytes() > cpd->dstBuf()->numBytes()) {
    std::string err = std::string("Destination buffer too small: ") + std::to_string(cpd->dstBuf()->numBytes()) + 
      ", required: " + std::to_string(cpd->srcBytes());
    return Nan::ThrowError(err.c_str());
  }
  obj->mWorker->doFrame(cpd, obj, new Nan::Callback(callback));

  info.GetReturnValue().Set(Nan::New(obj->mWorker->numQueued()));
}

NAN_METHOD(Concater::Quit) {
  if (info.Length() != 1)
    return Nan::ThrowError("Concater quit expects 1 argument");
  if (!info[0]->IsFunction())
    return Nan::ThrowError("Concater quit requires a valid callback as the parameter");
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  Concater* obj = Nan::ObjectWrap::Unwrap<Concater>(info.Holder());

  if (obj->mWorker != NULL)
    obj->mWorker->quit(callback);

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(Concater::Finish) {
  Concater* obj = Nan::ObjectWrap::Unwrap<Concater>(info.Holder());
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
