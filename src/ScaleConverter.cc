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
  ScaleConvertProcessData (Local<Object> srcBuf, uint32_t srcWidth, uint32_t srcHeight, std::string srcFmtCode, Local<Object> dstBuf,
                           bool unityFormat, bool unityScale)
    : mSrcBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(srcBuf), (uint32_t)node::Buffer::Length(srcBuf))),
      mSrcWidth(srcWidth), mSrcHeight(srcHeight), mSrcFmtCode(srcFmtCode), 
      mDstBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(dstBuf), (uint32_t)node::Buffer::Length(dstBuf))),
      mPacker(std::make_shared<Packers>(mSrcWidth, mSrcHeight, srcFmtCode, (uint32_t)node::Buffer::Length(srcBuf), "420P")),
      mUnityFormat(unityFormat), mUnityScale(unityScale), mConvertDstBuf(mDstBuf), mScaleSrcBuf(mSrcBuf)
  { 
    if (!mUnityFormat && !mUnityScale) {
      mConvertDstBuf = Memory::makeNew(getFormatBytes(mSrcFmtCode, mSrcWidth, mSrcHeight));
      mScaleSrcBuf = mConvertDstBuf;
      if (!mConvertDstBuf->buf())
        Nan::ThrowError("Failed to allocate buffer for packer result");
    }
  }
  ~ScaleConvertProcessData() { }
  
  std::shared_ptr<Memory> srcBuf() const { return mSrcBuf; }
  uint32_t srcWidth() const { return mSrcWidth; }
  uint32_t srcHeight() const { return mSrcHeight; }
  std::string srcFmtCode() const { return mSrcFmtCode; } 
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }
  std::shared_ptr<Packers> packer() const { return mPacker; }
  bool unityFormat() const { return mUnityFormat; }
  bool unityScale() const { return mUnityScale; }
  std::shared_ptr<Memory> convertDstBuf() const { return mConvertDstBuf; }
  std::shared_ptr<Memory> scaleSrcBuf() const { return mConvertDstBuf; }

private:
  std::shared_ptr<Memory> mSrcBuf;
  const uint32_t mSrcWidth;
  const uint32_t mSrcHeight;
  std::string mSrcFmtCode; 
  std::shared_ptr<Memory> mDstBuf;
  std::shared_ptr<Packers> mPacker;
  const bool mUnityFormat;
  const bool mUnityScale;
  std::shared_ptr<Memory> mConvertDstBuf;
  std::shared_ptr<Memory> mScaleSrcBuf;
};


ScaleConverter::ScaleConverter(std::string format, uint32_t width, uint32_t height) 
  : mFormat(format), mWidth(width), mHeight(height), mWorker(NULL),
    mScaleConverterFF(new ScaleConverterFF) {

  mDstBytesReq = getFormatBytes(mFormat, mWidth, mHeight);
  if (0 != mFormat.compare("420P")) {
    std::string err = std::string("Unsupported destination packing type \'") + mFormat + "\'";
    Nan::ThrowError(err.c_str());
  }
  if (!mWidth || (mWidth % 2) || !mHeight) {
    std::string err = std::string("Unsupported dimensions \'") + std::to_string(mWidth) + "x" + std::to_string(mHeight) + "\'";
    Nan::ThrowError(err.c_str());
  }
}
ScaleConverter::~ScaleConverter() {}

// iProcess
uint32_t ScaleConverter::processFrame (std::shared_ptr<iProcessData> processData) {
  Timer t;
  std::shared_ptr<ScaleConvertProcessData> scpd = std::dynamic_pointer_cast<ScaleConvertProcessData>(processData);

  if (scpd->unityFormat() && scpd->unityScale()) {
    memcpy (scpd->dstBuf()->buf(), scpd->srcBuf()->buf(), std::min<uint32_t>(scpd->dstBuf()->numBytes(), scpd->srcBuf()->numBytes()));
  }
  else {
    if (!scpd->unityFormat()) {
      scpd->packer()->convert(scpd->srcBuf(), scpd->convertDstBuf()); 
      printf("convert: %.2fms\n", t.delta());
    }
    
    if (!scpd->unityScale()) {
      mScaleConverterFF->scaleConvertFrame (scpd->scaleSrcBuf(), scpd->srcWidth(), scpd->srcHeight(), 0, scpd->dstBuf(), mWidth, mHeight, 0);
      printf("scale  : %.2fms\n", t.delta());
    }
  }
  return mDstBytesReq;
}

NAN_METHOD(ScaleConverter::Start) {
  if (info.Length() != 1)
    return Nan::ThrowError("ScaleConverter start expects 1 argument");
  if (!info[0]->IsFunction())
    return Nan::ThrowError("ScaleConverter start requires a valid callback as the parameter");
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  ScaleConverter* obj = Nan::ObjectWrap::Unwrap<ScaleConverter>(info.Holder());

  if (obj->mWorker != NULL)
    Nan::ThrowError("Attempt to restart encoder when not idle");
  
  obj->mWorker = new MyWorker(callback);
  AsyncQueueWorker(obj->mWorker);

  info.GetReturnValue().Set(Nan::New(obj->mDstBytesReq));
}

NAN_METHOD(ScaleConverter::ScaleConvert) {
  if (info.Length() != 6)
    return Nan::ThrowError("ScaleConverter ScaleConvert expects 6 arguments");
  if (!info[0]->IsArray())
    return Nan::ThrowError("ScaleConverter ScaleConvert requires a valid source buffer array as the first parameter");
  if (!info[4]->IsObject())
    return Nan::ThrowError("ScaleConverter ScaleConvert requires a valid destination buffer as the fifth parameter");
  if (!info[5]->IsFunction())
    return Nan::ThrowError("ScaleConverter ScaleConvert requires a valid callback as the sixth parameter");

  Local<Array> srcBufArray = Local<Array>::Cast(info[0]);
  uint32_t srcWidth = Nan::To<uint32_t>(info[1]).FromJust();
  uint32_t srcHeight = Nan::To<uint32_t>(info[2]).FromJust();
  v8::String::Utf8Value srcFmtString(Nan::To<v8::String>(info[3]).ToLocalChecked());
  Local<Object> dstBuf = Local<Object>::Cast(info[4]);
  Local<Function> callback = Local<Function>::Cast(info[5]);

  std::string srcFmtCode = *srcFmtString;
  if (srcFmtCode.compare("4175") && srcFmtCode.compare("v210")) {
    std::string err = std::string("Unsupported source format \'") + srcFmtCode.c_str() + "\'";
    return Nan::ThrowError(err.c_str());
  }
  if (1 != srcBufArray->Length()) {
    std::string err = std::string("ScaleConverter requires single source buffer - received ") + std::to_string(srcBufArray->Length());
    return Nan::ThrowError(err.c_str());
  }
  Local<Object> srcBuf = Local<Object>::Cast(srcBufArray->Get(0));

  ScaleConverter* obj = Nan::ObjectWrap::Unwrap<ScaleConverter>(info.Holder());
  if (obj->mWorker == NULL)
    return Nan::ThrowError("Attempt to scaleConvert when worker not started");

  if (obj->mDstBytesReq > node::Buffer::Length(dstBuf))
    return Nan::ThrowError("Insufficient destination buffer for specified format");

  bool unityFormat = (srcFmtCode == obj->mFormat);
  bool unityScale = ((srcWidth == obj->mWidth) && (srcHeight == obj->mHeight));
  std::shared_ptr<iProcessData> scpd = 
    std::make_shared<ScaleConvertProcessData>(srcBuf, srcWidth, srcHeight, srcFmtCode, dstBuf, unityFormat, unityScale);
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

NAN_METHOD(ScaleConverter::Finish) {
  ScaleConverter* obj = Nan::ObjectWrap::Unwrap<ScaleConverter>(info.Holder());
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
