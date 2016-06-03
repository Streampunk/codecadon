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

#ifndef SCALECONVERTER_H
#define SCALECONVERTER_H

#include "iProcess.h"
#include <memory>

namespace streampunk {

class MyWorker;
class ScaleConverterFF;
class Packers;
class EssenceInfo;

class ScaleConverter : public Nan::ObjectWrap, public iProcess {
public:
  static NAN_MODULE_INIT(Init);

  // iProcess
  uint32_t processFrame (std::shared_ptr<iProcessData> processData);
  
private:
  explicit ScaleConverter(Nan::Callback *callback);
  ~ScaleConverter();

  void doSetInfo(v8::Local<v8::Object> srcTags, v8::Local<v8::Object> dstTags);

  static NAN_METHOD(New) {
    if (info.IsConstructCall()) {
      if (!((info.Length() == 1) && (info[0]->IsFunction())))
        return Nan::ThrowError("Concater constructor requires a valid callback as the parameter");
      Nan::Callback *callback = new Nan::Callback(v8::Local<v8::Function>::Cast(info[0]));
      ScaleConverter *obj = new ScaleConverter(callback);
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    } else {
      const int argc = 3;
      v8::Local<v8::Value> argv[] = {info[0], info[1], info[2]};
      v8::Local<v8::Function> cons = Nan::New(constructor());
      info.GetReturnValue().Set(cons->NewInstance(argc, argv));
    }
  }

  static inline Nan::Persistent<v8::Function> & constructor() {
    static Nan::Persistent<v8::Function> my_constructor;
    return my_constructor;
  }

  static NAN_METHOD(SetInfo);
  static NAN_METHOD(ScaleConvert);
  static NAN_METHOD(Quit);

  MyWorker *mWorker;
  bool mSetInfoOK;
  bool mUnityPacking;
  bool mUnityScale;
  uint32_t mSrcFormatBytes;
  uint32_t mDstBytesReq;
  std::shared_ptr<EssenceInfo> mSrcVidInfo;
  std::shared_ptr<EssenceInfo> mDstVidInfo;
  std::shared_ptr<ScaleConverterFF> mScaleConverterFF;
  std::shared_ptr<Packers> mPacker;
};

} // namespace streampunk

#endif
