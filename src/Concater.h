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

#ifndef CONCATER_H
#define CONCATER_H

#include "iProcess.h"
#include <memory>

namespace streampunk {

class MyWorker;

class Concater : public Nan::ObjectWrap, public iProcess {
public:
  static NAN_MODULE_INIT(Init);

  // iProcess
  uint32_t processFrame (std::shared_ptr<iProcessData> processData);
  
private:
  explicit Concater(uint32_t numBytes);
  ~Concater();

  static NAN_METHOD(New) {
    if (info.IsConstructCall()) {
      uint32_t numBytes = Nan::To<uint32_t>(info[0]).FromJust();
      Concater *obj = new Concater(numBytes);
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    } else {
      const int argc = 1;
      v8::Local<v8::Value> argv[] = { info[0] };
      v8::Local<v8::Function> cons = Nan::New(constructor());
      info.GetReturnValue().Set(cons->NewInstance(argc, argv));
    }
  }

  static inline Nan::Persistent<v8::Function> & constructor() {
    static Nan::Persistent<v8::Function> my_constructor;
    return my_constructor;
  }

  static NAN_METHOD(Start);
  static NAN_METHOD(Concat);
  static NAN_METHOD(Quit);
  static NAN_METHOD(Finish);

  const uint32_t mNumBytes;
  MyWorker *mWorker;
};

} // namespace streampunk

#endif
