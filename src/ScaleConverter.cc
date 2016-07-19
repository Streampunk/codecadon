/* Copyright 2016 Streampunk Media Ltd.

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
#include "ScaleConverter.h"
#include "MyWorker.h"
#include "Timer.h"
#include "Packers.h"
#include "Memory.h"
#include "ScaleConverterFF.h"
#include "EssenceInfo.h"

#include <memory>

using namespace v8;

namespace streampunk {

class ScaleConvertProcessData : public iProcessData {
public:
  ScaleConvertProcessData (std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf, 
                           std::shared_ptr<Memory> convertDstBuf, std::shared_ptr<Memory> scaleSrcBuf)
    : mSrcBuf(srcBuf), mDstBuf(dstBuf), mConvertDstBuf(convertDstBuf), mScaleSrcBuf(scaleSrcBuf)
  { }
  ~ScaleConvertProcessData() { }
  
  std::shared_ptr<Memory> srcBuf() const { return mSrcBuf; }
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }
  std::shared_ptr<Memory> convertDstBuf() const { return mConvertDstBuf; }
  std::shared_ptr<Memory> scaleSrcBuf() const { return mScaleSrcBuf; }

private:
  std::shared_ptr<Memory> mSrcBuf;
  std::shared_ptr<Memory> mDstBuf;
  std::shared_ptr<Memory> mConvertDstBuf;
  std::shared_ptr<Memory> mScaleSrcBuf;
};

ScaleConverter::ScaleConverter(Nan::Callback *callback) 
  : mWorker(new MyWorker(callback)), mSetInfoOK(false), mUnityPacking(true), mUnityScale(true), mSrcFormatBytes(0), mDstBytesReq(0) {
  AsyncQueueWorker(mWorker);
}
ScaleConverter::~ScaleConverter() {}

// iProcess
uint32_t ScaleConverter::processFrame (std::shared_ptr<iProcessData> processData) {
  Timer t;
  std::shared_ptr<ScaleConvertProcessData> scpd = std::dynamic_pointer_cast<ScaleConvertProcessData>(processData);

  if (mUnityPacking && mUnityScale) {
    memcpy (scpd->dstBuf()->buf(), scpd->srcBuf()->buf(), std::min<uint32_t>(scpd->dstBuf()->numBytes(), scpd->srcBuf()->numBytes()));
  }
  else {
    if (!mUnityPacking) {
      mPacker->convert(scpd->srcBuf(), scpd->convertDstBuf()); 
      printf("convert: %.2fms\n", t.delta());
    }

    if (!mUnityScale) {
      mScaleConverterFF->scaleConvertFrame (scpd->scaleSrcBuf(), scpd->dstBuf());
      printf("scale  : %.2fms\n", t.delta());
    }
  }
  return mDstBytesReq;
}

void ScaleConverter::doSetInfo(Local<Object> srcTags, Local<Object> dstTags) {
  mSrcVidInfo = std::make_shared<EssenceInfo>(srcTags); 
  printf ("Converter SrcVidInfo: %s\n", mSrcVidInfo->toString().c_str());
  mDstVidInfo = std::make_shared<EssenceInfo>(dstTags); 
  printf("Converter DstVidInfo: %s\n", mDstVidInfo->toString().c_str());

  if (mSrcVidInfo->packing().compare("pgroup") && mSrcVidInfo->packing().compare("v210") && 
      mSrcVidInfo->packing().compare("YUV422P10") && mSrcVidInfo->packing().compare("UYVY10") && mSrcVidInfo->packing().compare("420P")) {
    std::string err = std::string("Unsupported source format \'") + mSrcVidInfo->packing() + "\'";
    return Nan::ThrowError(err.c_str());
  }
  if (mDstVidInfo->packing().compare("420P") && mDstVidInfo->packing().compare("YUV422P10")) {
    std::string err = std::string("Unsupported destination packing type \'") + mDstVidInfo->packing() + "\'";
    Nan::ThrowError(err.c_str());
  }
  if ((mSrcVidInfo->width() % 2) || (mDstVidInfo->width() % 2)) {
    std::string err = std::string("Width must be divisible by 2 - src ") + std::to_string(mSrcVidInfo->width()) + ", dst " + std::to_string(mDstVidInfo->width());
    Nan::ThrowError(err.c_str());
  }

  mScaleConverterFF = std::make_shared<ScaleConverterFF>(mSrcVidInfo, mDstVidInfo);
  mUnityPacking = (mSrcVidInfo->packing() == mScaleConverterFF->packingRequired());
  if (!mUnityPacking)
    mPacker = std::make_shared<Packers>(mSrcVidInfo->width(), mSrcVidInfo->height(), 
                                        mSrcVidInfo->packing(), mScaleConverterFF->packingRequired());
  mUnityScale = ((mSrcVidInfo->width() == mDstVidInfo->width()) && 
                 (mSrcVidInfo->height() == mDstVidInfo->height()) &&
                 (0==mSrcVidInfo->interlace().compare(mDstVidInfo->interlace())));
  mDstBytesReq = getFormatBytes(mDstVidInfo->packing(), mDstVidInfo->width(), mDstVidInfo->height());
}

NAN_METHOD(ScaleConverter::SetInfo) {
  if (info.Length() != 2)
    return Nan::ThrowError("Converter SetInfo expects 2 arguments");
  Local<Object> srcTags = Local<Object>::Cast(info[0]);
  Local<Object> dstTags = Local<Object>::Cast(info[1]);

  ScaleConverter* obj = Nan::ObjectWrap::Unwrap<ScaleConverter>(info.Holder());

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

NAN_METHOD(ScaleConverter::ScaleConvert) {
  if (info.Length() != 3)
    return Nan::ThrowError("ScaleConverter ScaleConvert expects 3 arguments");
  if (!info[0]->IsArray())
    return Nan::ThrowError("ScaleConverter ScaleConvert requires a valid source buffer array as the first parameter");
  if (!info[1]->IsObject())
    return Nan::ThrowError("ScaleConverter ScaleConvert requires a valid destination buffer as the second parameter");
  if (!info[2]->IsFunction())
    return Nan::ThrowError("ScaleConverter ScaleConvert requires a valid callback as the third parameter");

  Local<Array> srcBufArray = Local<Array>::Cast(info[0]);
  Local<Object> dstBufObj = Local<Object>::Cast(info[1]);
  Local<Function> callback = Local<Function>::Cast(info[2]);

  Local<Object> srcBufObj = Local<Object>::Cast(srcBufArray->Get(0));

  ScaleConverter* obj = Nan::ObjectWrap::Unwrap<ScaleConverter>(info.Holder());

  if (!obj->mSetInfoOK)
    return Nan::ThrowError("ScaleConvert called with incorrect setup parameters");

  obj->mSrcFormatBytes = getFormatBytes(obj->mSrcVidInfo->packing(), obj->mSrcVidInfo->width(), obj->mSrcVidInfo->height());
  if (obj->mSrcFormatBytes > (uint32_t)node::Buffer::Length(srcBufObj))
    Nan::ThrowError("Insufficient source buffer for conversion\n");

  if (obj->mDstBytesReq > node::Buffer::Length(dstBufObj))
    return Nan::ThrowError("Insufficient destination buffer for specified format");

  std::shared_ptr<Memory> srcBuf = Memory::makeNew((uint8_t *)node::Buffer::Data(srcBufObj), (uint32_t)node::Buffer::Length(srcBufObj));
  std::shared_ptr<Memory> dstBuf = Memory::makeNew((uint8_t *)node::Buffer::Data(dstBufObj), (uint32_t)node::Buffer::Length(dstBufObj));
  std::shared_ptr<Memory> convertDstBuf = dstBuf;
  std::shared_ptr<Memory> scaleSrcBuf = srcBuf;
  if (!obj->mUnityPacking && !obj->mUnityScale) {
    convertDstBuf = Memory::makeNew(getFormatBytes(obj->mScaleConverterFF->packingRequired(), obj->mSrcVidInfo->width(), obj->mSrcVidInfo->height()));
    scaleSrcBuf = convertDstBuf;
    if (!convertDstBuf->buf())
      Nan::ThrowError("Failed to allocate buffer for packer result");
  }

  std::shared_ptr<iProcessData> scpd = 
    std::make_shared<ScaleConvertProcessData>(srcBuf, dstBuf, convertDstBuf, scaleSrcBuf);
  obj->mWorker->doFrame(scpd, obj, new Nan::Callback(callback));

  info.GetReturnValue().Set(Nan::New(obj->mWorker->numQueued()));
}

NAN_METHOD(ScaleConverter::Quit) {
  if (info.Length() != 1)
    return Nan::ThrowError("ScaleConverter quit expects 1 argument");
  if (!info[0]->IsFunction())
    return Nan::ThrowError("ScaleConverter quit requires a valid callback as the parameter");
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  ScaleConverter* obj = Nan::ObjectWrap::Unwrap<ScaleConverter>(info.Holder());

  if (obj->mWorker != NULL)
    obj->mWorker->quit(callback);

  info.GetReturnValue().SetUndefined();
}

NAN_MODULE_INIT(ScaleConverter::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("ScaleConverter").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(tpl, "setInfo", SetInfo);
  SetPrototypeMethod(tpl, "scaleConvert", ScaleConvert);
  SetPrototypeMethod(tpl, "quit", Quit);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("ScaleConverter").ToLocalChecked(),
    Nan::GetFunction(tpl).ToLocalChecked());
}

} // namespace streampunk
