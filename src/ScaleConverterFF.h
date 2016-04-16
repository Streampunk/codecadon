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

#ifndef SCALECONVERTERFF_H
#define SCALECONVERTERFF_H

#include <memory>
struct SwsContext;

namespace streampunk {

class Memory;
class ScaleConverterFF {
public:
  ScaleConverterFF();
  ~ScaleConverterFF();

  void init(uint32_t srcWidth, uint32_t srcHeight, uint32_t srcPixFmt, 
            uint32_t dstWidth, uint32_t dstHeight, uint32_t dstPixFmt);
  void scaleConvertFrame (std::shared_ptr<Memory> srcBuf, 
                          uint32_t srcWidth, uint32_t srcHeight, uint32_t srcPixFmt,
                          std::shared_ptr<Memory> dstBuf, 
                          uint32_t dstWidth, uint32_t dstHeight, uint32_t dstPixFmt);

private:
  SwsContext *mSwsContext;
  uint32_t mSrcLinesize[4], mDstLinesize[4];
  uint32_t mSrcWidth;
  uint32_t mSrcHeight;
  uint32_t mSrcPixFmt;
  uint32_t mDstWidth;
  uint32_t mDstHeight;
  uint32_t mDstPixFmt;
};

} // namespace streampunk

#endif
