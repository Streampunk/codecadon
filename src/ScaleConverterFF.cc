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
#include "EssenceInfo.h"

extern "C" {
  #include <libavutil/imgutils.h>
  #include <libswscale/swscale.h>
}

namespace streampunk {

ScaleConverterFF::ScaleConverterFF(std::shared_ptr<EssenceInfo> srcVidInfo, std::shared_ptr<EssenceInfo> dstVidInfo)
  : mSwsContext(NULL), 
    mSrcWidth(srcVidInfo->width()), mSrcHeight(srcVidInfo->height()), mSrcIlace(srcVidInfo->interlace()),
    mSrcPixFmt((8==srcVidInfo->depth())?AV_PIX_FMT_YUV420P:AV_PIX_FMT_YUV422P10LE), 
    mDstWidth(dstVidInfo->width()), mDstHeight(dstVidInfo->height()), mDstIlace(dstVidInfo->interlace()),
    mDstPixFmt((8==dstVidInfo->depth())?AV_PIX_FMT_YUV420P:AV_PIX_FMT_YUV422P10LE) {

  uint32_t srcIshift = mSrcIlace.compare("prog")?1:0;
  uint32_t dstIshift = mDstIlace.compare("prog")?1:0;
  mSwsContext = sws_getContext(mSrcWidth, mSrcHeight>>srcIshift, (AVPixelFormat)mSrcPixFmt,
                               mDstWidth, mDstHeight>>dstIshift, (AVPixelFormat)mDstPixFmt,
                               SWS_BILINEAR, NULL, NULL, NULL);
  if (!mSwsContext) {
    fprintf(stderr,
      "Impossible to create scale context for the conversion "
      "fmt:%s s:%dx%d -> fmt:%s s:%dx%d\n",
      av_get_pix_fmt_name((AVPixelFormat)mSrcPixFmt), mSrcWidth, mSrcHeight,
      av_get_pix_fmt_name((AVPixelFormat)mDstPixFmt), mDstWidth, mDstHeight);
    Nan::ThrowError("Failed to create scale context");
    return;
  }

  mSrcLinesize[0] = (AV_PIX_FMT_YUV420P==mSrcPixFmt) ? mSrcWidth : mSrcWidth * 2;
  mSrcLinesize[1] = (AV_PIX_FMT_YUV420P==mSrcPixFmt) ? mSrcWidth / 2 : mSrcWidth;
  mSrcLinesize[2] = (AV_PIX_FMT_YUV420P==mSrcPixFmt) ? mSrcWidth / 2 : mSrcWidth;

  mDstLinesize[0] = (AV_PIX_FMT_YUV420P==mDstPixFmt) ? mDstWidth : mDstWidth * 2;
  mDstLinesize[1] = (AV_PIX_FMT_YUV420P==mDstPixFmt) ? mDstWidth / 2 : mDstWidth;
  mDstLinesize[2] = (AV_PIX_FMT_YUV420P==mDstPixFmt) ? mDstWidth / 2 : mDstWidth;
}

ScaleConverterFF::~ScaleConverterFF() {
  sws_freeContext(mSwsContext);
}

std::string ScaleConverterFF::packingRequired() const {
  return (AV_PIX_FMT_YUV420P==mSrcPixFmt)?"420P":"YUV422P10";
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

void ScaleConverterFF::scaleConvertFrame (std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf) { 
  uint8_t *srcData[4];
  uint32_t srcLumaBytes = mSrcLinesize[0] * mSrcHeight;
  uint32_t srcChromaBytes = mSrcLinesize[1] * mSrcHeight;
  if (AV_PIX_FMT_YUV420P==mSrcPixFmt)
    srcChromaBytes /= 2;
  srcData[0] = (uint8_t *)srcBuf->buf();
  srcData[1] = (uint8_t *)(srcBuf->buf() + srcLumaBytes);
  srcData[2] = (uint8_t *)(srcBuf->buf() + srcLumaBytes + srcChromaBytes);
  srcData[3] = NULL;

  uint8_t *dstData[4];
  uint32_t dstLumaBytes = mDstLinesize[0] * mDstHeight;
  uint32_t dstChromaBytes = mDstLinesize[1] * mDstHeight;
  if (AV_PIX_FMT_YUV420P==mDstPixFmt)
    dstChromaBytes /= 2;
  dstData[0] = (uint8_t *)dstBuf->buf();
  dstData[1] = (uint8_t *)(dstBuf->buf() + dstLumaBytes);
  dstData[2] = (uint8_t *)(dstBuf->buf() + dstLumaBytes + dstChromaBytes);
  dstData[3] = NULL;

  bool srcProgressive = (0 == mSrcIlace.compare("prog"));
  bool dstProgressive = (0 == mDstIlace.compare("prog"));
  if (srcProgressive && dstProgressive) {
    sws_scale(mSwsContext, (const uint8_t * const*)srcData,
              (const int *)mSrcLinesize, 0, mSrcHeight, dstData, (const int *)mDstLinesize);
  } else {
    bool srcTff = (0 == mSrcIlace.compare("tff"));
    bool dstTff = (0 == mDstIlace.compare("tff"));
    // first field
    scaleConvertField (srcData, dstData, srcTff?0:1, dstTff?0:1);
    // second field
    scaleConvertField (srcData, dstData, srcTff?1:0, dstTff?1:0);
  }
}

} // namespace streampunk
