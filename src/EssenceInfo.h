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

#ifndef ESSENCEINFO_H
#define ESSENCEINFO_H

#include <nan.h>
#include <sstream>

using namespace v8;

namespace streampunk {

class Duration {
public:
  Duration(uint32_t num, uint32_t den)
    : mNumerator(num), mDenominator(den) {}
    
  uint32_t numerator() const  { return mNumerator; }
  uint32_t denominator() const  { return mDenominator; }
  std::string toString() const  { return std::to_string(mNumerator) + "/" + std::to_string(mDenominator); }

private:
  const uint32_t mNumerator;
  const uint32_t mDenominator;  

  Duration();
  Duration(const Duration&);
};

class EssenceInfo {
public:
  EssenceInfo(Local<Object> tags)
    : mFormat(unpackStr(tags, "format", "video")),
      mIsVideo(0==mFormat.compare("video")),
      mEncodingName(unpackStr(tags, "encodingName", mIsVideo?"raw":"L16")), 
      mClockRate(unpackNum(tags, "clockRate", mIsVideo?90000:48000)),
      mWidth(mIsVideo?unpackNum(tags, "width", 1920):0),
      mHeight(mIsVideo?unpackNum(tags, "height", 1080):0),
      mSampling(mIsVideo?unpackStr(tags, "sampling", "YCbCr-4:2:2"):""),  
      mDepth(mIsVideo?unpackNum(tags, "depth", 8):0),
      mColorimetry(mIsVideo?unpackStr(tags, "colorimetry", "BT709-2"):""),
      mInterlace(mIsVideo?unpackBool(tags, "interlace", true)?"tff":"prog":""),
      mPacking(mIsVideo?unpackStr(tags, "packing", "pgroup"):""),
      mChannels(mIsVideo?0:unpackNum(tags, "channels", 2))
  {}
  ~EssenceInfo() {}

  std::string format() const  { return mFormat; }
  bool isVideo() const  { return mIsVideo; }
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
    if (mIsVideo)
      ss << "Video, " << mWidth << "x" << mHeight << ", " << (mInterlace.compare("prog")?"I":"P") << ", " << mPacking;
    else 
      ss << "Audio, " << "Clock Rate " << mClockRate << ", Channels " << mChannels << ", Encoding " << mEncodingName;
    return ss.str();
  }

private:
  std::string mFormat;
  bool mIsVideo;
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
};

} // namespace streampunk

#endif
