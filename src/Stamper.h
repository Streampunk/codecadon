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

#ifndef STAMPER_H
#define STAMPER_H

#include "iDebug.h"
#include "iProcess.h"
#include <memory>

namespace streampunk {

class MyWorker;
class EssenceInfo;
class WipeProcessData;
class CopyProcessData;
class MixProcessData;
class StampProcessData;

class Stamper : public Nan::ObjectWrap, public iProcess, public iDebug {
public:
  static NAN_MODULE_INIT(Init);

  // iProcess
  uint32_t processFrame (std::shared_ptr<iProcessData> processData);
  
private:
  explicit Stamper(Nan::Callback *callback);
  ~Stamper();

  void doSetInfo(v8::Local<v8::Array> srcTags, v8::Local<v8::Object> dstTags);
  void doWipe(std::shared_ptr<WipeProcessData> wpd);
  void doCopy(std::shared_ptr<CopyProcessData> cpd);
  void doMix(std::shared_ptr<MixProcessData> mpd);
  void doStamp(std::shared_ptr<StampProcessData> spd);
  
  static NAN_METHOD(New) {
    if (info.IsConstructCall()) {
      if (!((info.Length() == 1) && (info[0]->IsFunction())))
        return Nan::ThrowError("Stamper constructor requires a valid callback as the parameter");
      Nan::Callback *callback = new Nan::Callback(v8::Local<v8::Function>::Cast(info[0]));
      Stamper *obj = new Stamper(callback);
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    } else {
      const int argc = 1;
      v8::Local<v8::Value> argv[] = {info[0]};
      v8::Local<v8::Function> cons = Nan::New(constructor());
      info.GetReturnValue().Set(cons->NewInstance(Nan::GetCurrentContext(), argc, argv).ToLocalChecked());
    }
  }

  static inline Nan::Persistent<v8::Function> & constructor() {
    static Nan::Persistent<v8::Function> my_constructor;
    return my_constructor;
  }

  static NAN_METHOD(SetInfo);
  static NAN_METHOD(Wipe);
  static NAN_METHOD(Copy);
  static NAN_METHOD(Mix);
  static NAN_METHOD(Stamp);
  static NAN_METHOD(Quit);

  MyWorker *mWorker;
  bool mSetInfoOK;
  uint32_t mDstBytesReq;
  std::shared_ptr<EssenceInfo> mSrcVidInfo;
  std::shared_ptr<EssenceInfo> mDstVidInfo;
};

} // namespace streampunk

#endif
