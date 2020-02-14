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
#include "Decoder.h"
#include "MyWorker.h"
#include "Timer.h"
#include "Memory.h"
#include "DecoderFactory.h"
#include "EssenceInfo.h"
#include "Persist.h"

#include <memory>

using namespace v8;

namespace streampunk {

class DecodeProcessData : public iProcessData {
public:
  DecodeProcessData (Local<Object> srcBufObj, Local<Object> dstBufObj)
    : mPersistentSrcBuf(new Persist(srcBufObj)),
      mPersistentDstBuf(new Persist(dstBufObj)),
      mSrcBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(srcBufObj), (uint32_t)node::Buffer::Length(srcBufObj))),
      mDstBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(dstBufObj), (uint32_t)node::Buffer::Length(dstBufObj)))
    { }
  ~DecodeProcessData() { }
  
  std::shared_ptr<Memory> srcBuf() const { return mSrcBuf; }
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }

private:
  std::unique_ptr<Persist> mPersistentSrcBuf;
  std::unique_ptr<Persist> mPersistentDstBuf;
  std::shared_ptr<Memory> mSrcBuf;
  std::shared_ptr<Memory> mDstBuf;
};


Decoder::Decoder(Nan::Callback *callback) 
  : mWorker(new MyWorker(callback)), mFrameNum(0), mSetInfoOK(false) {
  AsyncQueueWorker(mWorker);
}
Decoder::~Decoder() {}

// iProcess
uint32_t Decoder::processFrame (std::shared_ptr<iProcessData> processData) {
  Timer t;
  std::shared_ptr<DecodeProcessData> dpd = std::dynamic_pointer_cast<DecodeProcessData>(processData);

  // do the decode
  uint32_t dstBytes = 0;
  mDecoderDriver->decodeFrame (dpd->srcBuf(), dpd->dstBuf(), mFrameNum++, &dstBytes);
  printDebug(eDebug, "decode : %.2fms\n", t.delta());

  return dstBytes;
}

void Decoder::doSetInfo(Local<Object> srcTags, Local<Object> dstTags) {
  mSrcVidInfo = std::make_shared<EssenceInfo>(srcTags); 
  printDebug(eInfo, "Decoder SrcVidInfo: %s\n", mSrcVidInfo->toString().c_str());
  mDstVidInfo = std::make_shared<EssenceInfo>(dstTags); 
  printDebug(eInfo, "Decoder DstVidInfo: %s\n", mDstVidInfo->toString().c_str());

  if (mSrcVidInfo->encodingName().compare("h264") && mSrcVidInfo->encodingName().compare("vp8") && 
      mSrcVidInfo->encodingName().compare("AVCi50") && mSrcVidInfo->encodingName().compare("AVCi100")) {
    std::string err = std::string("Unsupported source encoding \'") + mSrcVidInfo->encodingName().c_str() + "\'";
    return Nan::ThrowError(err.c_str());
  }
  if (mDstVidInfo->packing().compare("420P") && mDstVidInfo->packing().compare("UYVY10")) {
    std::string err = std::string("Unsupported packing type \'") + mDstVidInfo->packing() + "\'";
    Nan::ThrowError(err.c_str());
    return;
  }
  if ((mSrcVidInfo->width() % 2) || (mDstVidInfo->width() % 2)) {
    std::string err = std::string("Width must be divisible by 2 - src ") + std::to_string(mSrcVidInfo->width()) + ", dst " + std::to_string(mDstVidInfo->width());
    Nan::ThrowError(err.c_str());
  }

  try {
    mDecoderDriver = DecoderFactory::createDecoder(mSrcVidInfo, mDstVidInfo);
  }
  catch (std::exception& err) {
    Nan::ThrowError(err.what());
  }
}

NAN_METHOD(Decoder::SetInfo) {
  if (info.Length() != 3)
    return Nan::ThrowError("Decoder SetInfo expects 3 arguments");
  if (!info[0]->IsObject())
    return Nan::ThrowError("Decoder SetInfo requires a valid source info object as the first parameter");
  if (!info[1]->IsObject())
    return Nan::ThrowError("Decoder SetInfo requires a valid destination info object as the second parameter");
  if (!info[2]->IsNumber())
    return Nan::ThrowError("Decoder SetInfo requires a valid debug level as the third parameter");
  Local<Object> srcTags = Local<Object>::Cast(info[0]);
  Local<Object> dstTags = Local<Object>::Cast(info[1]);

  Decoder* obj = Nan::ObjectWrap::Unwrap<Decoder>(info.Holder());
  obj->setDebug((eDebugLevel)Nan::To<uint32_t>(info[2]).FromJust());
  
  Nan::TryCatch try_catch;
  obj->doSetInfo(srcTags, dstTags);
  if (try_catch.HasCaught()) {
    obj->mSetInfoOK = false;
    try_catch.ReThrow();
    return;
  }

  obj->mSetInfoOK = true;
  info.GetReturnValue().Set(Nan::New(obj->mDecoderDriver->bytesReq()));
}

NAN_METHOD(Decoder::Decode) {
  if (info.Length() != 3)
    return Nan::ThrowError("Decoder Decode expects 3 arguments");
  if (!info[0]->IsArray())
    return Nan::ThrowError("Decoder Decode requires a valid source buffer array as the first parameter");
  if (!info[1]->IsObject())
    return Nan::ThrowError("Decoder Decode requires a valid destination buffer as the second parameter");
  if (!info[2]->IsFunction())
    return Nan::ThrowError("Decoder Decode requires a valid callback as the third parameter");

  Local<Array> srcBufArray = Local<Array>::Cast(info[0]);
  Local<Object> dstBuf = Local<Object>::Cast(info[1]);
  Local<Function> callback = Local<Function>::Cast(info[2]);

  Decoder* obj = Nan::ObjectWrap::Unwrap<Decoder>(info.Holder());

  if (!obj->mSetInfoOK)
    return Nan::ThrowError("Decoder decode called with incorrect setup parameters");

  if (1 != srcBufArray->Length()) {
    std::string err = std::string("Decoder requires single source buffer - received ") + std::to_string(srcBufArray->Length());
    return Nan::ThrowError(err.c_str());
  }
  Local<Object> srcBuf = Local<Object>::Cast(srcBufArray->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), 0).ToLocalChecked());

  std::shared_ptr<iProcessData> epd = std::make_shared<DecodeProcessData>(srcBuf, dstBuf);
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

NAN_MODULE_INIT(Decoder::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Decoder").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(tpl, "setInfo", SetInfo);
  SetPrototypeMethod(tpl, "decode", Decode);
  SetPrototypeMethod(tpl, "quit", Quit);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("Decoder").ToLocalChecked(),
    Nan::GetFunction(tpl).ToLocalChecked());
}

} // namespace streampunk
