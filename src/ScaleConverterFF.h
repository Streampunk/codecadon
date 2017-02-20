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

#ifndef SCALECONVERTERFF_H
#define SCALECONVERTERFF_H

#include <memory>
#include "Primitives.h"
struct SwsContext;

namespace streampunk {

class Memory;
class EssenceInfo;
class ScaleConverterFF {
public:
  ScaleConverterFF(std::shared_ptr<EssenceInfo> srcVidInfo, std::shared_ptr<EssenceInfo> dstVidInfo,
                   const fXY &userScale, const fXY &userDstOffset);
  ~ScaleConverterFF();

  std::string packingRequired() const;
  void scaleConvertFrame(std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf); 

private:
  SwsContext *mSwsContext;
  const uint32_t mSrcWidth;
  const uint32_t mSrcHeight;
  const std::string mSrcIlace;
  const uint32_t mSrcPixFmt;
  const uint32_t mDstWidth;
  const uint32_t mDstHeight;
  const std::string mDstIlace;
  const uint32_t mDstPixFmt;
  const fXY mUserScale;
  const fXY mUserDstOffset;
  fXY mScale;
  fXY mDstOffset;
  bool mDoWipe;
  uint32_t mSrcLinesize[4], mDstLinesize[4];

  void scaleConvertField (uint8_t **srcData, uint8_t **dstData, uint32_t srcField, uint32_t dstField);
};

} // namespace streampunk

#endif
