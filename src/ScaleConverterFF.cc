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

  for (uint32_t i = 0; i < 4; ++i) {
    mSrcLinesize[i] = 0;
    mDstLinesize[i] = 0;
  }
}

ScaleConverterFF::~ScaleConverterFF() {
  sws_freeContext(mSwsContext);
}

void ScaleConverterFF::init(uint32_t srcWidth, uint32_t srcHeight, uint32_t srcPixFmt, std::string srcIlace, 
                            uint32_t dstWidth, uint32_t dstHeight, uint32_t dstPixFmt, std::string dstIlace) {
  mSrcWidth = srcWidth;
  mSrcHeight = srcHeight;
  mSrcPixFmt = srcPixFmt;
  mSrcIlace = srcIlace;
  mDstWidth = dstWidth;
  mDstHeight = dstHeight;
  mDstPixFmt = dstPixFmt;
  mDstIlace = dstIlace;

  if (mSwsContext) sws_freeContext(mSwsContext);
  uint32_t ishift = (srcIlace.compare("prog") || dstIlace.compare("prog"))?1:0;
  mSwsContext = sws_getContext(mSrcWidth, mSrcHeight>>ishift, (AVPixelFormat)mSrcPixFmt,
                               mDstWidth, mDstHeight>>ishift, (AVPixelFormat)mDstPixFmt,
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

void ScaleConverterFF::scaleConvertField (uint8_t **srcData, uint8_t **dstData, uint32_t srcField, uint32_t dstField) {
  const uint8_t *srcBuf[4];
  uint8_t *dstBuf[4];
  uint32_t srcStride[4], dstStride[4];

  for (uint32_t i = 0; i < 4; ++i) {
    srcStride[i] = mSrcLinesize[i] * 2;
    dstStride[i] = mDstLinesize[i] * 2;
    srcBuf[i] = srcData[i] + srcField * mSrcLinesize[i];
    dstBuf[i] = dstData[i] + dstField * mDstLinesize[i];
  }

  sws_scale(mSwsContext, srcBuf, (const int *)srcStride, 0, mSrcHeight/2, dstBuf, (const int *)dstStride);
}

void ScaleConverterFF::scaleConvertFrame (std::shared_ptr<Memory> srcBuf, 
                                          uint32_t srcWidth, uint32_t srcHeight, uint32_t srcPixFmt, std::string srcIlace,
                                          std::shared_ptr<Memory> dstBuf, 
                                          uint32_t dstWidth, uint32_t dstHeight, uint32_t dstPixFmt, std::string dstIlace) {

  if ((mSrcWidth != srcWidth) || (mSrcHeight != srcHeight) || (mSrcPixFmt != srcPixFmt) || (mSrcIlace.compare(srcIlace)) ||
      (mDstWidth != dstWidth) || (mDstHeight != dstHeight) || (mDstPixFmt != dstPixFmt) || (mDstIlace.compare(dstIlace))) {
    init(srcWidth, srcHeight, srcPixFmt, srcIlace, dstWidth, dstHeight, dstPixFmt, dstIlace);      
  }

  uint8_t *srcData[4];
  uint32_t srcLumaBytes = mSrcLinesize[0] * mSrcHeight;
  srcData[0] = (uint8_t *)srcBuf->buf();
  srcData[1] = (uint8_t *)(srcBuf->buf() + srcLumaBytes);
  srcData[2] = (uint8_t *)(srcBuf->buf() + srcLumaBytes + srcLumaBytes / 4);
  srcData[3] = NULL;

  uint8_t *dstData[4];
  uint32_t dstLumaBytes = mDstLinesize[0] * mDstHeight;
  dstData[0] = (uint8_t *)dstBuf->buf();
  dstData[1] = (uint8_t *)(dstBuf->buf() + dstLumaBytes);
  dstData[2] = (uint8_t *)(dstBuf->buf() + dstLumaBytes + dstLumaBytes / 4);
  dstData[3] = NULL;

  bool srcProgressive = (0 == srcIlace.compare("prog"));
  bool dstProgressive = (0 == dstIlace.compare("prog"));
  if (srcProgressive && dstProgressive) {
    sws_scale(mSwsContext, (const uint8_t * const*)srcData,
              (const int *)mSrcLinesize, 0, mSrcHeight, dstData, (const int *)mDstLinesize);
  } else {
    bool srcTff = (0 == srcIlace.compare("tff"));
    bool dstTff = (0 == dstIlace.compare("tff"));
    // first field
    scaleConvertField (srcData, dstData, srcTff?0:1, dstTff?0:1);
    // second field
    scaleConvertField (srcData, dstData, srcTff?1:0, dstTff?1:0);
  }
}

} // namespace streampunk
