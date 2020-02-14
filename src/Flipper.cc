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
#include "Flipper.h"
#include "MyWorker.h"
#include "Timer.h"
#include "Packers.h"
#include "Memory.h"
#include "EssenceInfo.h"
#include "Persist.h"

#include <memory>

using namespace v8;

namespace streampunk {

class FlipProcessData : public iProcessData {
public:
  FlipProcessData (Local<Object> srcBufObj, Local<Object> dstBufObj)
    : mPersistentSrcBuf(new Persist(srcBufObj)), mPersistentDstBuf(new Persist(dstBufObj)),
      mSrcBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(srcBufObj), (uint32_t)node::Buffer::Length(srcBufObj))),
      mDstBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(dstBufObj), (uint32_t)node::Buffer::Length(dstBufObj)))
  {}
  ~FlipProcessData() {}
  
  std::shared_ptr<Memory> srcBuf() const { return mSrcBuf; }
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }

private:
  std::unique_ptr<Persist> mPersistentSrcBuf;
  std::unique_ptr<Persist> mPersistentDstBuf;
  std::shared_ptr<Memory> mSrcBuf;
  std::shared_ptr<Memory> mDstBuf;
};

class FlipInfo {
public:
  FlipInfo(Local<Object> tags)
    : mHflip(unpackBool(tags, "h", false)),
      mVflip(unpackBool(tags, "v", false))
  {}
  ~FlipInfo() {}

  bool hflip() const  { return mHflip; }
  bool vflip() const  { return mVflip; }
  
private:
  bool mHflip;
  bool mVflip;

  bool unpackBool(Local<Object> tags, const std::string& key, bool dflt) {
    Local<String> keyStr = Nan::New<String>(key).ToLocalChecked();
    if (!Nan::Has(tags, keyStr).FromJust())
      return dflt;
      
    Local<Boolean> val = Local<Boolean>::Cast(Nan::Get(tags, keyStr).ToLocalChecked());
    return Nan::True() == val;
  }
};

Flipper::Flipper(Nan::Callback *callback) 
  : mWorker(new MyWorker(callback)), mSetInfoOK(false), mPitchBytes(0), mInterlace(false), mTff(true) {
  AsyncQueueWorker(mWorker);
}
Flipper::~Flipper() {}

// iProcess
uint32_t Flipper::processFrame (std::shared_ptr<iProcessData> processData) {
  Timer t;
  std::shared_ptr<FlipProcessData> fpd = std::dynamic_pointer_cast<FlipProcessData>(processData);

  std::shared_ptr<Memory> srcBuf = fpd->srcBuf();
  std::shared_ptr<Memory> dstBuf = fpd->dstBuf();
  for (uint32_t dstY=0, srcY=mSrcVidInfo->height()-1; dstY != mSrcVidInfo->height(); ++dstY, --srcY) {
    const uint8_t* srcLine = srcBuf->buf() + mPitchBytes * srcY;
    uint8_t* dstLine = dstBuf->buf() + mPitchBytes * dstY;   
    memcpy(dstLine, srcLine, mPitchBytes);
  }

  printDebug(eDebug, "flip : %.2fms\n", t.delta());
  return mSrcFormatBytes;
}

NAN_METHOD(Flipper::SetInfo) {
  if (info.Length() != 3)
    return Nan::ThrowError("Flipper SetInfo expects 3 arguments");
  if (!info[0]->IsObject())
    return Nan::ThrowError("Flipper SetInfo requires a valid source info object as the first parameter");
  if (!info[1]->IsObject())
    return Nan::ThrowError("Flipper SetInfo requires a valid flip info object as the second parameter");
  if (!info[2]->IsNumber())
    return Nan::ThrowError("Flipper SetInfo requires a valid debug level as the third parameter");
  Local<Object> srcTags = Local<Object>::Cast(info[0]);
  Local<Object> flipObj = Local<Object>::Cast(info[1]);
  
  Flipper* obj = Nan::ObjectWrap::Unwrap<Flipper>(info.Holder());
  obj->setDebug((eDebugLevel)Nan::To<uint32_t>(info[2]).FromJust());
  
  obj->mSrcVidInfo = std::make_shared<EssenceInfo>(srcTags);
  obj->mFlipInfo = std::make_shared<FlipInfo>(flipObj);
  obj->printDebug(eInfo, "Flipper SrcVidInfo: %s%s%s\n", obj->mSrcVidInfo->toString().c_str(), obj->mFlipInfo->hflip()?", hflip":"", obj->mFlipInfo->vflip()?", vflip":"");

  // Currently supporting only non-planar formats
  if (obj->mSrcVidInfo->packing().compare("pgroup") && obj->mSrcVidInfo->packing().compare("v210") && 
      obj->mSrcVidInfo->packing().compare("UYVY10") && 
      obj->mSrcVidInfo->packing().compare("RGBA8") && obj->mSrcVidInfo->packing().compare("BGRA8") && 
      obj->mSrcVidInfo->packing().compare("BGR10-A") && obj->mSrcVidInfo->packing().compare("BGR10-A-BS")) {
    std::string err = std::string("Unsupported source format \'") + obj->mSrcVidInfo->packing() + "\'";
    return Nan::ThrowError(err.c_str());
  }

  obj->mSrcFormatBytes = getFormatBytes(obj->mSrcVidInfo->packing(), obj->mSrcVidInfo->width(), obj->mSrcVidInfo->height());
  obj->mPitchBytes = obj->mSrcVidInfo->width() * 4;
  if (0==obj->mSrcVidInfo->packing().compare("pgroup"))
    obj->mPitchBytes = obj->mSrcVidInfo->width() * 5 / 2;
  else if (0==obj->mSrcVidInfo->packing().compare("v210"))
    obj->mPitchBytes = (((obj->mSrcVidInfo->width() + 47) / 48) * 48 * 8 / 3);

  obj->mInterlace = (0!=obj->mSrcVidInfo->interlace().compare("prog"));
  obj->mTff = (0==obj->mSrcVidInfo->interlace().compare("tff"));

  obj->mSetInfoOK = true;
  info.GetReturnValue().Set(Nan::New(obj->mSrcFormatBytes));
}

NAN_METHOD(Flipper::Flip) {
  if (info.Length() != 3)
    return Nan::ThrowError("Flipper flip expects 3 arguments");
  if (!info[0]->IsArray())
    return Nan::ThrowError("Flipper flip requires a valid source buffer array as the first parameter");
  if (!info[1]->IsObject())
    return Nan::ThrowError("Flipper flip requires a valid destination buffer as the second parameter");
  if (!info[2]->IsFunction())
    return Nan::ThrowError("Flipper flip requires a valid callback as the third parameter");
  Local<Array> srcBufArray = Local<Array>::Cast(info[0]);
  Local<Object> dstBufObj = Local<Object>::Cast(info[1]);
  Local<Function> callback = Local<Function>::Cast(info[2]);

  Local<Object> srcBufObj = Local<Object>::Cast(srcBufArray->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), 0).ToLocalChecked());

  Flipper* obj = Nan::ObjectWrap::Unwrap<Flipper>(info.Holder());

  if (!obj->mSetInfoOK)
    return Nan::ThrowError("Flipper flip called with incorrect setup parameters");

  obj->mSrcFormatBytes = getFormatBytes(obj->mSrcVidInfo->packing(), obj->mSrcVidInfo->width(), obj->mSrcVidInfo->height());
  if (obj->mSrcFormatBytes > (uint32_t)node::Buffer::Length(srcBufObj))
    return Nan::ThrowError("Insufficient source buffer for conversion");

  if (obj->mSrcFormatBytes > node::Buffer::Length(dstBufObj))
    return Nan::ThrowError("Insufficient destination buffer for specified format");

  std::shared_ptr<FlipProcessData> fpd = std::make_shared<FlipProcessData>(srcBufObj, dstBufObj);
  obj->mWorker->doFrame(fpd, obj, new Nan::Callback(callback));

  info.GetReturnValue().Set(Nan::New(obj->mWorker->numQueued()));
}

NAN_METHOD(Flipper::Quit) {
  if (info.Length() != 1)
    return Nan::ThrowError("Flipper quit expects 1 argument");
  if (!info[0]->IsFunction())
    return Nan::ThrowError("Flipper quit requires a valid callback as the parameter");
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  Flipper* obj = Nan::ObjectWrap::Unwrap<Flipper>(info.Holder());

  if (obj->mWorker != NULL)
    obj->mWorker->quit(callback);

  info.GetReturnValue().SetUndefined();
}

NAN_MODULE_INIT(Flipper::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Flipper").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(tpl, "setInfo", SetInfo);
  SetPrototypeMethod(tpl, "flip", Flip);
  SetPrototypeMethod(tpl, "quit", Quit);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("Flipper").ToLocalChecked(),
    Nan::GetFunction(tpl).ToLocalChecked());
}

} // namespace streampunk
