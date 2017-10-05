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

#ifndef FLIPPER_H
#define FLIPPER_H

#include <nan.h>
#include "iProcess.h"
#include <memory>

namespace streampunk {

class MyWorker;
class EssenceInfo;
class FlipInfo;

class Flipper : public Nan::ObjectWrap, public iProcess {
public:
  static NAN_MODULE_INIT(Init);

  // iProcess
  uint32_t processFrame (std::shared_ptr<iProcessData> processData);
  
private:
  explicit Flipper(Nan::Callback *callback);
  ~Flipper();

  static NAN_METHOD(New) {
    if (info.IsConstructCall()) {
      if (!((info.Length() == 1) && (info[0]->IsFunction())))
        return Nan::ThrowError("Flipper constructor requires a valid callback as the parameter");
      Nan::Callback *callback = new Nan::Callback(v8::Local<v8::Function>::Cast(info[0]));
      Flipper *obj = new Flipper(callback);
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    } else {
      const int argc = 1;
      v8::Local<v8::Value> argv[] = { info[0] };
      v8::Local<v8::Function> cons = Nan::New(constructor());
      info.GetReturnValue().Set(cons->NewInstance(Nan::GetCurrentContext(), argc, argv).ToLocalChecked());
    }
  }

  static inline Nan::Persistent<v8::Function> & constructor() {
    static Nan::Persistent<v8::Function> my_constructor;
    return my_constructor;
  }

  static NAN_METHOD(SetInfo);
  static NAN_METHOD(Flip);
  static NAN_METHOD(Quit);

  MyWorker *mWorker;
  bool mSetInfoOK;
  uint32_t mPitchBytes;
  uint32_t mSrcFormatBytes;
  std::shared_ptr<EssenceInfo> mSrcVidInfo;
  std::shared_ptr<FlipInfo> mFlipInfo;
  bool mInterlace;
  bool mTff;
};

} // namespace streampunk

#endif
