/* Copyright 2017 Streampunk Media Ltd.

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
#include "Packer.h"
#include "MyWorker.h"
#include "Timer.h"
#include "Packers.h"
#include "Memory.h"
#include "EssenceInfo.h"
#include "Persist.h"

#include <memory>

using namespace v8;

namespace streampunk {

class PackerProcessData : public iProcessData {
public:
  PackerProcessData (Local<Object> srcBufObj, Local<Object> dstBufObj)
    : mPersistentSrcBuf(new Persist(srcBufObj)),
      mPersistentDstBuf(new Persist(dstBufObj)),
      mSrcBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(srcBufObj), (uint32_t)node::Buffer::Length(srcBufObj))),
      mDstBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(dstBufObj), (uint32_t)node::Buffer::Length(dstBufObj)))
  { }
  ~PackerProcessData() { }
  
  std::shared_ptr<Memory> srcBuf() const { return mSrcBuf; }
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }

private:
  std::unique_ptr<Persist> mPersistentSrcBuf;
  std::unique_ptr<Persist> mPersistentDstBuf;
  std::shared_ptr<Memory> mSrcBuf;
  std::shared_ptr<Memory> mDstBuf;
};

Packer::Packer(Nan::Callback *callback) 
  : mWorker(new MyWorker(callback)), mSetInfoOK(false), mUnityPacking(true), mSrcFormatBytes(0), mDstBytesReq(0) {
  AsyncQueueWorker(mWorker);
}
Packer::~Packer() {}

// iProcess
uint32_t Packer::processFrame (std::shared_ptr<iProcessData> processData) {
  Timer t;
  std::shared_ptr<PackerProcessData> ppd = std::dynamic_pointer_cast<PackerProcessData>(processData);

  if (mUnityPacking) {
    memcpy (ppd->dstBuf()->buf(), ppd->srcBuf()->buf(), ppd->srcBuf()->numBytes());
  }
  else {
    mPacker->convert(ppd->srcBuf(), ppd->dstBuf()); 
    printDebug(eDebug, "pack: %.2fms\n", t.delta());
  }
  return mDstBytesReq;
}

void Packer::doSetInfo(Local<Object> srcTags, Local<Object> dstTags) {
  mSrcVidInfo = std::make_shared<EssenceInfo>(srcTags); 
  printDebug(eInfo, "Packer SrcVidInfo: %s\n", mSrcVidInfo->toString().c_str());
  mDstVidInfo = std::make_shared<EssenceInfo>(dstTags); 
  printDebug(eInfo, "Packer DstVidInfo: %s\n", mDstVidInfo->toString().c_str());

  if (mSrcVidInfo->packing().compare("pgroup") && mSrcVidInfo->packing().compare("v210") && 
      mSrcVidInfo->packing().compare("YUV422P10") && mSrcVidInfo->packing().compare("UYVY10") && 
      mSrcVidInfo->packing().compare("420P")) {
    std::string err = std::string("Unsupported source format \'") + mSrcVidInfo->packing() + "\'";
    return Nan::ThrowError(err.c_str());
  }
  if (mDstVidInfo->packing().compare("420P") && mDstVidInfo->packing().compare("YUV422P10") && 
      mDstVidInfo->packing().compare("UYVY10") && mDstVidInfo->packing().compare("pgroup") && mDstVidInfo->packing().compare("v210")) {
    std::string err = std::string("Unsupported destination packing type \'") + mDstVidInfo->packing() + "\'";
    Nan::ThrowError(err.c_str());
  }
  if ((mSrcVidInfo->width() % 2) || (mDstVidInfo->width() % 2)) {
    std::string err = std::string("Width must be divisible by 2 - src ") + std::to_string(mSrcVidInfo->width()) + ", dst " + std::to_string(mDstVidInfo->width());
    Nan::ThrowError(err.c_str());
  }

  mPacker = std::make_shared<Packers>(mSrcVidInfo->width(), mSrcVidInfo->height(), 
                                      mSrcVidInfo->packing(), mDstVidInfo->packing());
  mUnityPacking = (mSrcVidInfo->packing() == mDstVidInfo->packing());
  mDstBytesReq = getFormatBytes(mDstVidInfo->packing(), mDstVidInfo->width(), mDstVidInfo->height());
}

NAN_METHOD(Packer::SetInfo) {
  if (info.Length() != 3)
    return Nan::ThrowError("Packer SetInfo expects 3 arguments");
  if (!info[0]->IsObject())
    return Nan::ThrowError("Packer SetInfo requires a valid source info object as the first parameter");
  if (!info[1]->IsObject())
    return Nan::ThrowError("Packer SetInfo requires a valid destination info object as the second parameter");
  if (!info[2]->IsNumber())
    return Nan::ThrowError("Packer SetInfo requires a valid debug level as the third parameter");
  Local<Object> srcTags = Local<Object>::Cast(info[0]);
  Local<Object> dstTags = Local<Object>::Cast(info[1]);

  Packer* obj = Nan::ObjectWrap::Unwrap<Packer>(info.Holder());
  obj->setDebug((eDebugLevel)Nan::To<uint32_t>(info[2]).FromJust());
  
  Nan::TryCatch try_catch;
  obj->doSetInfo(srcTags, dstTags);
  if (try_catch.HasCaught()) {
    obj->mSetInfoOK = false;
    try_catch.ReThrow();
    return;
  }

  obj->mSetInfoOK = true;
  info.GetReturnValue().Set(Nan::New(obj->mDstBytesReq));
}

NAN_METHOD(Packer::Pack) {
  if (info.Length() != 3)
    return Nan::ThrowError("Packer Pack expects 3 arguments");
  if (!info[0]->IsArray())
    return Nan::ThrowError("Packer Pack requires a valid source buffer array as the first parameter");
  if (!info[1]->IsObject())
    return Nan::ThrowError("Packer Pack requires a valid destination buffer as the second parameter");
  if (!info[2]->IsFunction())
    return Nan::ThrowError("Packer Pack requires a valid callback as the third parameter");

  Local<Array> srcBufArray = Local<Array>::Cast(info[0]);
  Local<Object> dstBufObj = Local<Object>::Cast(info[1]);
  Local<Function> callback = Local<Function>::Cast(info[2]);

  Local<Object> srcBufObj = Local<Object>::Cast(srcBufArray->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), 0).ToLocalChecked());

  Packer* obj = Nan::ObjectWrap::Unwrap<Packer>(info.Holder());

  if (!obj->mSetInfoOK)
    return Nan::ThrowError("Pack called with incorrect setup parameters");

  obj->mSrcFormatBytes = getFormatBytes(obj->mSrcVidInfo->packing(), obj->mSrcVidInfo->width(), obj->mSrcVidInfo->height());
  if (obj->mSrcFormatBytes > (uint32_t)node::Buffer::Length(srcBufObj))
    Nan::ThrowError("Insufficient source buffer for conversion\n");

  if (obj->mDstBytesReq > node::Buffer::Length(dstBufObj))
    return Nan::ThrowError("Insufficient destination buffer for specified format");

  std::shared_ptr<iProcessData> ppd = 
    std::make_shared<PackerProcessData>(srcBufObj, dstBufObj);
  obj->mWorker->doFrame(ppd, obj, new Nan::Callback(callback));

  info.GetReturnValue().Set(Nan::New(obj->mWorker->numQueued()));
}

NAN_METHOD(Packer::Quit) {
  if (info.Length() != 1)
    return Nan::ThrowError("Packer quit expects 1 argument");
  if (!info[0]->IsFunction())
    return Nan::ThrowError("Packer quit requires a valid callback as the parameter");
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  Packer* obj = Nan::ObjectWrap::Unwrap<Packer>(info.Holder());

  if (obj->mWorker != NULL)
    obj->mWorker->quit(callback);

  info.GetReturnValue().SetUndefined();
}

NAN_MODULE_INIT(Packer::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Packer").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(tpl, "setInfo", SetInfo);
  SetPrototypeMethod(tpl, "pack", Pack);
  SetPrototypeMethod(tpl, "quit", Quit);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("Packer").ToLocalChecked(),
    Nan::GetFunction(tpl).ToLocalChecked());
}

} // namespace streampunk
