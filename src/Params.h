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

#ifndef PARAMS_H
#define PARAMS_H

#include <nan.h>

using namespace v8;

namespace streampunk {

class Params {
public:
  bool isVideo() const  { return mIsVideo; } 

protected:
  const bool mIsVideo;

  Params(bool isVideo) : mIsVideo(isVideo) {}
  virtual ~Params() {}

  std::string unpackValue(Local<Object> tags, const std::string& key) {
    Local<String> keyStr = Nan::New<String>(key).ToLocalChecked();
    if (!Nan::Has(tags, keyStr).FromJust())
      return std::string();
      
    Local<Array> valueArray = Local<Array>::Cast(Nan::Get(tags, keyStr).ToLocalChecked());
    return *String::Utf8Value(valueArray->Get(0));
  }

  bool unpackBool(Local<Object> tags, const std::string& key, bool dflt) {
    std::string val = unpackValue(tags, key);
    bool result = dflt;
    if (!val.empty()) {
      if ((0==val.compare("1")) || (0==val.compare("true")))
        result = true;
      else if ((0==val.compare("0")) || (0==val.compare("false")))
        result = false;
    }
    return result;
  }

  uint32_t unpackNum(Local<Object> tags, const std::string& key, uint32_t dflt) {
    std::string val = unpackValue(tags, key);
    return val.empty()?dflt:std::stoi(val);
  } 

  std::string unpackStr(Local<Object> tags, const std::string& key, std::string dflt) {
    std::string val = unpackValue(tags, key);
    return val.empty()?dflt:val;
  } 

private:
  Params();
  Params(const Params &);
};

} // namespace streampunk

#endif
