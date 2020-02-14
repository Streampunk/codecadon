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
#include "Encoder.h"
#include "MyWorker.h"
#include "Timer.h"
#include "Packers.h"
#include "Memory.h"
#include "EncoderFactory.h"
#include "EssenceInfo.h"
#include "Persist.h"

#include <memory>

using namespace v8;

namespace streampunk {

uint32_t beToLe32 (uint32_t be) {
  uint8_t *pB = (uint8_t *)&be;
  return (pB[0] << 24) | (pB[1] << 16) | (pB[2] << 8) | pB[3];
}

class EncodeProcessData : public iProcessData {
public:
  EncodeProcessData (Local<Object> srcBufObj, Local<Object> dstBufObj, std::shared_ptr<Memory> convertDstBuf)
    : mPersistentSrcBuf(new Persist(srcBufObj)),
      mPersistentDstBuf(new Persist(dstBufObj)),
      mSrcBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(srcBufObj), (uint32_t)node::Buffer::Length(srcBufObj))), 
      mDstBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(dstBufObj), (uint32_t)node::Buffer::Length(dstBufObj))), 
      mConvertDstBuf(convertDstBuf)
    { }
  ~EncodeProcessData() {}
  
  std::shared_ptr<Memory> srcBuf() const { return mSrcBuf; }
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }
  std::shared_ptr<Memory> convertDstBuf() const { return mConvertDstBuf; }

private:
  std::unique_ptr<Persist> mPersistentSrcBuf;
  std::unique_ptr<Persist> mPersistentDstBuf;
  std::shared_ptr<Memory> mSrcBuf;
  std::shared_ptr<Memory> mDstBuf;
  std::shared_ptr<Memory> mConvertDstBuf;
};


Encoder::Encoder(Nan::Callback *callback) 
  : mWorker(new MyWorker(callback)), mFrameNum(0), mSetInfoOK(false) {
  AsyncQueueWorker(mWorker);
}
Encoder::~Encoder() {}

// iProcess
uint32_t Encoder::processFrame (std::shared_ptr<iProcessData> processData) {
  Timer t;
  std::shared_ptr<EncodeProcessData> epd = std::dynamic_pointer_cast<EncodeProcessData>(processData);

  // do the encode
  uint32_t dstBytes = 0;
  std::shared_ptr<Memory> encodeSrcBuf = epd->srcBuf();
  if (mPacker) {
    mPacker->convert(epd->srcBuf(), epd->convertDstBuf());
    encodeSrcBuf = epd->convertDstBuf();
    printDebug(eDebug, "convert: %.2fms\n", t.delta());
  }

  try {
    mEncoderDriver->encodeFrame (encodeSrcBuf, epd->dstBuf(), mFrameNum++, &dstBytes);
  } catch (std::exception& err) {
    printDebug(eError, "Encode error: %s\n", err.what());
  }
  printDebug(eDebug, "encode: %.2fms\n", t.delta());

  return dstBytes;
}

void Encoder::doSetInfo(Local<Object> srcTags, Local<Object> dstTags, const Duration& duration,
                        Local<Object> encodeTags) {
  mSrcInfo = std::make_shared<EssenceInfo>(srcTags); 
  printDebug(eInfo, "Encoder SrcInfo: %s\n", mSrcInfo->toString().c_str());
  mDstInfo = std::make_shared<EssenceInfo>(dstTags); 
  printDebug(eInfo, "Encoder DstInfo: %s\n", mDstInfo->toString().c_str());
  std::shared_ptr<EncodeParams> encodeParams = std::make_shared<EncodeParams>(encodeTags, mSrcInfo->isVideo()); 
  printDebug(eInfo, "Encode Params: %s\n", encodeParams->toString().c_str());

  if (mSrcInfo->isVideo()) {
    if (mSrcInfo->packing().compare("420P") && mSrcInfo->packing().compare("YUV422P10") && 
        mSrcInfo->packing().compare("pgroup") && mSrcInfo->packing().compare("v210")) {
      std::string err = std::string("Unsupported source format \'") + mSrcInfo->packing().c_str() + "\'";
      return Nan::ThrowError(err.c_str());
    }
    if (mDstInfo->packing().compare("h264") && mDstInfo->packing().compare("vp8") && 
        mDstInfo->packing().compare("AVCi50") && mDstInfo->packing().compare("AVCi100")) {
      std::string err = std::string("Unsupported codec type \'") + mDstInfo->packing() + "\'";
      Nan::ThrowError(err.c_str());
      return;
    }
    if ((mSrcInfo->width() % 2) || (mDstInfo->width() % 2)) {
      std::string err = std::string("Width must be divisible by 2 - src ") + std::to_string(mSrcInfo->width()) + ", dst " + std::to_string(mDstInfo->width());
      Nan::ThrowError(err.c_str());
    }
    if ((mSrcInfo->width() != mDstInfo->width()) || (mSrcInfo->height() != mDstInfo->height())) {
      std::string err = std::string("Unsupported dimensions ") +
        std::to_string(mSrcInfo->width()) + "x" + std::to_string(mSrcInfo->height()) + " != " +
        std::to_string(mDstInfo->width()) + "x" + std::to_string(mDstInfo->height());
        return Nan::ThrowError(err.c_str());
    }
  }
  else {
    if (mDstInfo->encodingName().compare("AAC")) {
      std::string err = std::string("Unsupported audio codec type \'") + mDstInfo->encodingName() + "\'";
      Nan::ThrowError(err.c_str());
      return;
    }
  }

  try {
    mEncoderDriver = EncoderFactory::createEncoder(mSrcInfo, mDstInfo, duration, encodeParams);
  } catch (std::exception& err) {
    return Nan::ThrowError(err.what());
  }
  if (mSrcInfo->isVideo() && mEncoderDriver->packingRequired().compare(mSrcInfo->packing()))
    mPacker = std::make_shared<Packers>(mSrcInfo->width(), mSrcInfo->height(), mSrcInfo->packing(), mEncoderDriver->packingRequired());
}

NAN_METHOD(Encoder::SetInfo) {
  if (info.Length() != 5)
    return Nan::ThrowError("Encoder SetInfo expects 5 arguments");
  if (!info[0]->IsObject())
    return Nan::ThrowError("Encoder SetInfo requires a valid source info object as the first parameter");
  if (!info[1]->IsObject())
    return Nan::ThrowError("Encoder SetInfo requires a valid destination info object as the second parameter");
  if (!info[2]->IsObject())
    return Nan::ThrowError("Encoder SetInfo requires a valid duration buffer as the third parameter");
  if (!info[3]->IsObject())
    return Nan::ThrowError("Encoder SetInfo requires a valid params object as the fourth parameter");
  if (!info[4]->IsNumber())
    return Nan::ThrowError("Encoder SetInfo requires a valid debug level as the fifth parameter");
  Local<Object> srcTags = Local<Object>::Cast(info[0]);
  Local<Object> dstTags = Local<Object>::Cast(info[1]);
  Local<Object> durObj = Local<Object>::Cast(info[2]);
  Local<Object> encodeTags = Local<Object>::Cast(info[3]);
  
  Encoder* obj = Nan::ObjectWrap::Unwrap<Encoder>(info.Holder());
  obj->setDebug((eDebugLevel)Nan::To<uint32_t>(info[4]).FromJust());
  
  uint32_t *pDur = (uint32_t *)node::Buffer::Data(durObj);
  uint32_t durNum = beToLe32(*pDur++);
  uint32_t durDen = beToLe32(*pDur);
  Duration duration(durNum, durDen);

  Nan::TryCatch try_catch;
  obj->doSetInfo(srcTags, dstTags, duration, encodeTags);
  if (try_catch.HasCaught()) {
    obj->mSetInfoOK = false;
    try_catch.ReThrow();
    return;
  }

  obj->mSetInfoOK = true;
  info.GetReturnValue().Set(Nan::New(obj->mEncoderDriver->bytesReq()));
}

NAN_METHOD(Encoder::Encode) {
  if (info.Length() != 3)
    return Nan::ThrowError("Encoder Encode expects 3 arguments");
  if (!info[0]->IsArray())
    return Nan::ThrowError("Encoder Encode requires a valid source buffer array as the first parameter");
  if (!info[1]->IsObject())
    return Nan::ThrowError("Encoder Encode requires a valid destination buffer as the second parameter");
  if (!info[2]->IsFunction())
    return Nan::ThrowError("Encoder Encode requires a valid callback as the third parameter");

  Local<Array> srcBufArray = Local<Array>::Cast(info[0]);
  Local<Object> dstBufObj = Local<Object>::Cast(info[1]);
  Local<Function> callback = Local<Function>::Cast(info[2]);

  Encoder* obj = Nan::ObjectWrap::Unwrap<Encoder>(info.Holder());

  if (!obj->mSetInfoOK)
    return Nan::ThrowError("Encoder Encode called with incorrect setup parameters");

  if (1 != srcBufArray->Length()) {
    std::string err = std::string("Encoder requires single source buffer - received ") + std::to_string(srcBufArray->Length());
    return Nan::ThrowError(err.c_str());
  }
  Local<Object> srcBufObj = Local<Object>::Cast(srcBufArray->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), 0).ToLocalChecked());
  std::shared_ptr<Memory> convertDstBuf;
  if (obj->mPacker)
    convertDstBuf = Memory::makeNew(getFormatBytes(obj->mEncoderDriver->packingRequired(), obj->mSrcInfo->width(), obj->mSrcInfo->height()));
  std::shared_ptr<iProcessData> epd = std::make_shared<EncodeProcessData>(srcBufObj, dstBufObj, convertDstBuf);
  obj->mWorker->doFrame(epd, obj, new Nan::Callback(callback));

  info.GetReturnValue().Set(Nan::New(obj->mWorker->numQueued()));
}

NAN_METHOD(Encoder::Quit) {
  if (info.Length() != 1)
    return Nan::ThrowError("Encoder quit expects 1 argument");
  if (!info[0]->IsFunction())
    return Nan::ThrowError("Encoder quit requires a valid callback as the parameter");
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  Encoder* obj = Nan::ObjectWrap::Unwrap<Encoder>(info.Holder());

  if (obj->mWorker != NULL)
    obj->mWorker->quit(callback);

  info.GetReturnValue().SetUndefined();
}

NAN_MODULE_INIT(Encoder::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Encoder").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(tpl, "setInfo", SetInfo);
  SetPrototypeMethod(tpl, "encode", Encode);
  SetPrototypeMethod(tpl, "quit", Quit);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("Encoder").ToLocalChecked(),
    Nan::GetFunction(tpl).ToLocalChecked());
}

} // namespace streampunk
