/* Copyright 2015 Christine S. MacNeill

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by appli cable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef DECODER_H
#define DECODER_H

#include "iProcess.h"
#include <memory>

namespace streampunk {

class MyWorker;
class DecoderFF;

class Decoder : public Nan::ObjectWrap, public iProcess {
public:
  static NAN_MODULE_INIT(Init);

  // iProcess
  uint32_t processFrame (std::shared_ptr<iProcessData> processData);
  
private:
  explicit Decoder(std::string format, uint32_t width, uint32_t height);
  ~Decoder();

  static NAN_METHOD(New) {
    if (info.IsConstructCall()) {
      v8::String::Utf8Value format(Nan::To<v8::String>(info[0]).ToLocalChecked());
      uint32_t width = info[1]->IsUndefined() ? 0 : Nan::To<uint32_t>(info[1]).FromJust();
      uint32_t height = info[2]->IsUndefined() ? 0 : Nan::To<uint32_t>(info[2]).FromJust();
      Decoder *obj = new Decoder(*format, width, height);
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

  static NAN_METHOD(Start);
  static NAN_METHOD(Decode);
  static NAN_METHOD(Quit);
  static NAN_METHOD(Finish);

  const std::string mFormat;
  const uint32_t mWidth;
  const uint32_t mHeight;
  MyWorker *mWorker;
  DecoderFF *mDecoder;
  uint32_t mFrameNum;
};

} // namespace streampunk

#endif
