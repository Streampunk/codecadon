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
#include "Concater.h"
#include "MyWorker.h"
#include "Timer.h"
#include "Packers.h"
#include "Memory.h"
#include "EssenceInfo.h"
#include "Persist.h"

#include <memory>

using namespace v8;

namespace streampunk {

class ConcatProcessData : public iProcessData {
public:
  ConcatProcessData (Local<Array> srcBufArray, Local<Object> dstBuf)
    : mPersistentSrcBuf(new Persist(srcBufArray)), mPersistentDstBuf(new Persist(dstBuf)),
      mDstBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(dstBuf), (uint32_t)node::Buffer::Length(dstBuf))), mSrcBytes(0) {
    for (uint32_t i = 0; i < srcBufArray->Length(); ++i) {
      Local<Object> bufferObj = Local<Object>::Cast(srcBufArray->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), i).ToLocalChecked());
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
  std::unique_ptr<Persist> mPersistentSrcBuf;
  std::unique_ptr<Persist> mPersistentDstBuf;
  tBufVec mSrcBufVec;
  std::shared_ptr<Memory> mDstBuf;
  uint32_t mSrcBytes;
};


Concater::Concater(Nan::Callback *callback) 
  : mWorker(new MyWorker(callback)), mSetInfoOK(false), mIsVideo(true), mPitchBytes(0), mInterlace(false), mTff(true) {
  AsyncQueueWorker(mWorker);
}
Concater::~Concater() {}

// iProcess
uint32_t Concater::processFrame (std::shared_ptr<iProcessData> processData) {
  Timer t;
  std::shared_ptr<ConcatProcessData> cpd = std::dynamic_pointer_cast<ConcatProcessData>(processData);

  tBufVec srcBufVec = cpd->srcBufVec();
  std::shared_ptr<Memory> dstBuf = cpd->dstBuf();
  uint32_t totalBytes = 0;
  uint32_t concatBufOffset = 0; 
  uint32_t xBytes = 0;
  uint32_t y = 0;

  for (tBufVec::const_iterator it = srcBufVec.begin(); it != srcBufVec.end(); ++it) {
    const uint8_t* srcBuf = it->first;
    uint32_t len = it->second;
    totalBytes += len;

    if (mIsVideo && mInterlace) {
      uint32_t yStep = 2 * mPitchBytes;
      uint32_t secondFieldStartLine = mSrcEssInfo->height() / 2;
      
      while (len) {
        bool firstField = y < secondFieldStartLine;
        uint32_t yBytes = yStep * (firstField ? y : (y - secondFieldStartLine));
        if (mTff)
          yBytes += firstField?0:mPitchBytes;
        else
          yBytes += firstField?mPitchBytes:0;
        concatBufOffset = xBytes + yBytes;

        uint32_t thisLen = len;
        if (xBytes + len >= mPitchBytes)
          thisLen = mPitchBytes - xBytes;

        if (concatBufOffset + thisLen > dstBuf->numBytes())
          thisLen = dstBuf->numBytes() - concatBufOffset;
  
        memcpy (dstBuf->buf() + concatBufOffset, srcBuf, thisLen);

        len -= thisLen;
        srcBuf += thisLen;
        xBytes = (xBytes + thisLen) % mPitchBytes;
        if (0 == xBytes)
          y++;
      }
    } else {
      if (concatBufOffset + len > dstBuf->numBytes())
        len = dstBuf->numBytes() - concatBufOffset;

      memcpy (dstBuf->buf() + concatBufOffset, srcBuf, len);
      concatBufOffset += len;
    }
  }
  
  printDebug(eDebug, "concat: %.2fms\n", t.delta());
  return totalBytes;
}

NAN_METHOD(Concater::SetInfo) {
  if (info.Length() != 2)
    return Nan::ThrowError("Concater SetInfo expects 2 arguments");
  if (!info[0]->IsObject())
    return Nan::ThrowError("Concater SetInfo requires a valid source info object as the first parameter");
  if (!info[1]->IsNumber())
    return Nan::ThrowError("Concater SetInfo requires a valid debug level as the second parameter");
  Local<Object> srcTags = Local<Object>::Cast(info[0]);
  
  Concater* obj = Nan::ObjectWrap::Unwrap<Concater>(info.Holder());
  obj->setDebug((eDebugLevel)Nan::To<uint32_t>(info[1]).FromJust());

  obj->mSrcEssInfo = std::make_shared<EssenceInfo>(srcTags); 
  obj->printDebug(eInfo, "Concater EssInfo: %s\n", obj->mSrcEssInfo->toString().c_str());

  uint32_t sampleBytes = 0;
  obj->mIsVideo = obj->mSrcEssInfo->isVideo();
  if (obj->mIsVideo) {
    if (0==obj->mSrcEssInfo->packing().compare("pgroup"))
      obj->mPitchBytes = obj->mSrcEssInfo->width() * 5 / 2;
    else
      obj->mPitchBytes = (((obj->mSrcEssInfo->width() + 47) / 48) * 48 * 8 / 3);

    sampleBytes = obj->mPitchBytes * obj->mSrcEssInfo->height();

    obj->mInterlace = (0!=obj->mSrcEssInfo->interlace().compare("prog"));
    obj->mTff = (0==obj->mSrcEssInfo->interlace().compare("tff"));
  }
  else {
    sampleBytes = obj->mSrcEssInfo->channels() * std::stoi(obj->mSrcEssInfo->encodingName().c_str() + 1) / 8;
  }

  obj->mSetInfoOK = true;
  info.GetReturnValue().Set(Nan::New(sampleBytes));
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

  if (!obj->mSetInfoOK)
    return Nan::ThrowError("Concater Concat called with incorrect setup parameters");

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

NAN_MODULE_INIT(Concater::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Concater").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(tpl, "setInfo", SetInfo);
  SetPrototypeMethod(tpl, "concat", Concat);
  SetPrototypeMethod(tpl, "quit", Quit);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("Concater").ToLocalChecked(),
    Nan::GetFunction(tpl).ToLocalChecked());
}

} // namespace streampunk
