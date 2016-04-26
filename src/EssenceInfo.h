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

#ifndef ESSENCEINFO_H
#define ESSENCEINFO_H

#include <nan.h>
#include <sstream>

using namespace v8;

namespace streampunk {

class EssenceInfo {
public:
  EssenceInfo(Local<Object> tags)
    : mFormat(unpackStr(tags, "format", "video")),
      mEncodingName(unpackStr(tags, "encodingName", "raw")), 
      mClockRate(unpackNum(tags, "clockRate", 90000)),
      mWidth(unpackNum(tags, "width", 1920)),
      mHeight(unpackNum(tags, "height", 1080)),
      mSampling(unpackStr(tags, "sampling", "YCbCr-4:2:2")),  
      mDepth(unpackNum(tags, "depth", 8)),
      mColorimetry(unpackStr(tags, "colorimetry", "BT709-2")),
      mInterlace(unpackNum(tags, "interlace", 1)?"tff":"prog"),
      mPacking(unpackStr(tags, "packing", "pgroup")),
      mChannels(unpackNum(tags, "channels", 0))
  {}
  ~EssenceInfo() {}

  std::string format() const  { return mFormat; }
  std::string encodingName() const  { return mEncodingName; }
  uint32_t clockRate() const  { return mClockRate; }
  uint32_t width() const  { return mWidth; }
  uint32_t height() const  { return mHeight; }
  std::string sampling() const  { return mSampling; }
  uint32_t depth() const  { return mDepth; }
  std::string colorimetry() const  { return mColorimetry; }
  std::string interlace() const  { return mInterlace; }
  std::string packing() const  { return mPacking; }
  uint32_t channels() const  { return mChannels; }

  std::string toString() const  { 
    std::stringstream ss;
    ss << mWidth << "x" << mHeight << ", " << (mInterlace.compare("prog")?"I":"P") << ", " << mPacking; 
    return ss.str();
  }

private:
  std::string mFormat;
  std::string mEncodingName;
  uint32_t mClockRate;
  uint32_t mWidth;
  uint32_t mHeight;
  std::string mSampling;
  uint32_t mDepth;
  std::string mColorimetry;
  std::string mInterlace;
  std::string mPacking;
  uint32_t mChannels;

  std::string unpackValue(Local<Object> tags, const std::string& key) {
    Local<String> keyStr = Nan::New<String>(key).ToLocalChecked();
    if (!Nan::Has(tags, keyStr).FromJust())
      return std::string();
      
    Local<Array> valueArray = Local<Array>::Cast(Nan::Get(tags, keyStr).ToLocalChecked());
    return *String::Utf8Value(valueArray->Get(0));
  }

  uint32_t unpackNum(Local<Object> tags, const std::string& key, uint32_t dflt) {
    std::string val = unpackValue(tags, key);
    return val.empty()?dflt:std::stoi(val);
  } 

  std::string unpackStr(Local<Object> tags, const std::string& key, std::string dflt) {
    std::string val = unpackValue(tags, key);
    return val.empty()?dflt:val;
  } 
};

} // namespace streampunk

#endif
