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

#include <nan.h>
#include "ScaleConverterFF.h"
#include "Memory.h"

extern "C" {
  #include <libavutil/imgutils.h>
  #include <libswscale/swscale.h>
}

namespace streampunk {

ScaleConverterFF::ScaleConverterFF() 
  : mSwsContext(NULL), 
    mSrcWidth(0), mSrcHeight(0), mSrcPixFmt(0),
    mDstWidth(0), mDstHeight(0), mDstPixFmt(0) {
}

ScaleConverterFF::~ScaleConverterFF() {
  sws_freeContext(mSwsContext);
}

void ScaleConverterFF::init(uint32_t srcWidth, uint32_t srcHeight, uint32_t srcPixFmt, 
                          uint32_t dstWidth, uint32_t dstHeight, uint32_t dstPixFmt) {
  mSrcWidth = srcWidth;
  mSrcHeight = srcHeight;
  mSrcPixFmt = srcPixFmt;
  mDstWidth = dstWidth;
  mDstHeight = dstHeight;
  mDstPixFmt = dstPixFmt;

  if (mSwsContext) sws_freeContext(mSwsContext);
  mSwsContext = sws_getContext(mSrcWidth, mSrcHeight, (AVPixelFormat)mSrcPixFmt,
                               mDstWidth, mDstHeight, (AVPixelFormat)mDstPixFmt,
                               SWS_BILINEAR, NULL, NULL, NULL);
  if (!mSwsContext) {
    fprintf(stderr,
      "Impossible to create scale context for the conversion "
      "fmt:%s s:%dx%d -> fmt:%s s:%dx%d\n",
      av_get_pix_fmt_name((AVPixelFormat)mSrcPixFmt), mSrcWidth, mSrcHeight,
      av_get_pix_fmt_name((AVPixelFormat)mDstPixFmt), mDstWidth, mDstHeight);
    return Nan::ThrowError("Failed to create scale context");
  }

  mSrcLinesize[0] = mSrcWidth;
  mSrcLinesize[1] = mSrcWidth / 2;
  mSrcLinesize[2] = mSrcWidth / 2;

  mDstLinesize[0] = mDstWidth;
  mDstLinesize[1] = mDstWidth / 2;
  mDstLinesize[2] = mDstWidth / 2;
}

void ScaleConverterFF::scaleConvertFrame (std::shared_ptr<Memory> srcBuf, 
                                          uint32_t srcWidth, uint32_t srcHeight, uint32_t srcPixFmt,
                                          std::shared_ptr<Memory> dstBuf, 
                                          uint32_t dstWidth, uint32_t dstHeight, uint32_t dstPixFmt) {

  if ((mSrcWidth != srcWidth) || (mSrcHeight != srcHeight) || (mSrcPixFmt != srcPixFmt) ||
      (mDstWidth != dstWidth) || (mDstHeight != dstHeight) || (mDstPixFmt != dstPixFmt)) {
    init(srcWidth, srcHeight, srcPixFmt, dstWidth, dstHeight, dstPixFmt);      
  }

  uint8_t *srcData[4];
  uint32_t srcLumaBytes = mSrcLinesize[0] * mSrcHeight;
  srcData[0] = (uint8_t *)srcBuf->buf();
  srcData[1] = (uint8_t *)(srcBuf->buf() + srcLumaBytes);
  srcData[2] = (uint8_t *)(srcBuf->buf() + srcLumaBytes + srcLumaBytes / 4);

  uint8_t *dstData[4];
  uint32_t dstLumaBytes = mDstLinesize[0] * mDstHeight;
  dstData[0] = (uint8_t *)dstBuf->buf();
  dstData[1] = (uint8_t *)(dstBuf->buf() + dstLumaBytes);
  dstData[2] = (uint8_t *)(dstBuf->buf() + dstLumaBytes + dstLumaBytes / 4);

  sws_scale(mSwsContext, (const uint8_t * const*)srcData,
            (const int *)mSrcLinesize, 0, mSrcHeight, dstData, (const int *)mDstLinesize);
}

} // namespace streampunk
