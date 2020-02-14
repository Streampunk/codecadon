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

#include <nan.h>
#include "Stamper.h"
#include "MyWorker.h"
#include "Timer.h"
#include "Memory.h"
#include "EssenceInfo.h"
#include "Packers.h"
#include "Primitives.h"
#include "Persist.h"

#include <memory>

using namespace v8;

namespace streampunk {

class WipeProcessData : public iProcessData {
public:
  WipeProcessData (Local<Object> dstBufObj, const iRect &wipeRect, const fCol &wipeCol)
    : mPersistentDstBuf(new Persist(dstBufObj)),
      mDstBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(dstBufObj), (uint32_t)node::Buffer::Length(dstBufObj))),
      mWipeRect(wipeRect), mWipeCol(wipeCol)
  { }
  ~WipeProcessData() { }
  
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }
  iRect wipeRect() const { return mWipeRect; }
  fCol wipeCol() const { return mWipeCol; }

private:
  std::unique_ptr<Persist> mPersistentDstBuf;
  std::shared_ptr<Memory> mDstBuf;
  iRect mWipeRect;
  fCol mWipeCol;
};

class CopyProcessData : public iProcessData {
public:
  CopyProcessData (Local<Object> srcBufObj, Local<Object> dstBufObj, const iXY &dstOrg)
    : mPersistentSrcBuf(new Persist(srcBufObj)),
      mPersistentDstBuf(new Persist(dstBufObj)),
      mSrcBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(srcBufObj), (uint32_t)node::Buffer::Length(srcBufObj))),
      mDstBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(dstBufObj), (uint32_t)node::Buffer::Length(dstBufObj))),
      mDstOrg(dstOrg)
  { }
  ~CopyProcessData() { }
  
  std::shared_ptr<Memory> srcBuf() const { return mSrcBuf; }
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }
  iXY dstOrg() const { return mDstOrg; }

private:
  std::unique_ptr<Persist> mPersistentSrcBuf;
  std::unique_ptr<Persist> mPersistentDstBuf;
  std::shared_ptr<Memory> mSrcBuf;
  std::shared_ptr<Memory> mDstBuf;
  iXY mDstOrg;
};

class MixProcessData : public iProcessData {
public:
  MixProcessData (Local<Array> srcBufArray, Local<Object> dstBufObj, float pressure)
    : mPersistentDstBuf(new Persist(dstBufObj)),
      mDstBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(dstBufObj), (uint32_t)node::Buffer::Length(dstBufObj))),
      mPressure(pressure)
  { 
    for (uint32_t i=0; i<srcBufArray->Length(); ++i) {
      Local<Object> srcBufObj = Local<Object>::Cast(srcBufArray->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), i).ToLocalChecked());
      mPersistentSrcBufs.push_back(std::shared_ptr<Persist>(new Persist(srcBufObj)));
      mSrcBufs.push_back(Memory::makeNew((uint8_t *)node::Buffer::Data(srcBufObj), (uint32_t)node::Buffer::Length(srcBufObj)));
    }
  }
  ~MixProcessData() { }
  
  std::vector<std::shared_ptr<Memory> > srcBufs() const { return mSrcBufs; }
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }
  float pressure() const { return mPressure; }

private:
  std::vector<std::shared_ptr<Persist> > mPersistentSrcBufs;
  std::unique_ptr<Persist> mPersistentDstBuf;
  std::vector<std::shared_ptr<Memory> > mSrcBufs;
  std::shared_ptr<Memory> mDstBuf;
  float mPressure;
};

class StampProcessData : public iProcessData {
public:
  StampProcessData (Local<Array> srcBufArray, Local<Object> dstBufObj)
    : mPersistentDstBuf(new Persist(dstBufObj)),
      mDstBuf(Memory::makeNew((uint8_t *)node::Buffer::Data(dstBufObj), (uint32_t)node::Buffer::Length(dstBufObj)))
  { 
    for (uint32_t i=0; i<srcBufArray->Length(); ++i) {
      Local<Object> srcBufObj = Local<Object>::Cast(srcBufArray->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), i).ToLocalChecked());
      mPersistentSrcBufs.push_back(std::shared_ptr<Persist>(new Persist(srcBufObj)));
      mSrcBufs.push_back(Memory::makeNew((uint8_t *)node::Buffer::Data(srcBufObj), (uint32_t)node::Buffer::Length(srcBufObj)));
    }
  }
  ~StampProcessData() { }
  
  std::vector<std::shared_ptr<Memory> > srcBufs() const { return mSrcBufs; }
  std::shared_ptr<Memory> dstBuf() const { return mDstBuf; }

private:
  std::vector<std::shared_ptr<Persist> > mPersistentSrcBufs;
  std::unique_ptr<Persist> mPersistentDstBuf;
  std::vector<std::shared_ptr<Memory> > mSrcBufs;
  std::shared_ptr<Memory> mDstBuf;
};

Stamper::Stamper(Nan::Callback *callback) 
  : mWorker(new MyWorker(callback)), mSetInfoOK(false), mDstBytesReq(0) {
  AsyncQueueWorker(mWorker);
}
Stamper::~Stamper() {}

// iProcess
uint32_t Stamper::processFrame (std::shared_ptr<iProcessData> processData) {
  Timer t;
  std::string func("null");
  std::shared_ptr<WipeProcessData> wpd = std::dynamic_pointer_cast<WipeProcessData>(processData);
  if (wpd) {
    func = "wipe";
    doWipe(wpd);
  }

  std::shared_ptr<CopyProcessData> cpd = std::dynamic_pointer_cast<CopyProcessData>(processData);
  if (cpd) {
    func = "copy";
    doCopy(cpd);
  }

  std::shared_ptr<MixProcessData> mpd = std::dynamic_pointer_cast<MixProcessData>(processData);
  if (mpd) {
    func = "mix";
    doMix(mpd);
  }

  std::shared_ptr<StampProcessData> spd = std::dynamic_pointer_cast<StampProcessData>(processData);
  if (spd) {
    func = "stamp";
    doStamp(spd);
  }

  printDebug(eDebug, "%s: %.2fms\n", func.c_str(), t.delta());

  return mDstBytesReq;
}

void Stamper::doSetInfo(Local<Array> srcTagsArray, Local<Object> dstTags) {
  Local<Object> srcTags = Local<Object>::Cast(srcTagsArray->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), 0).ToLocalChecked());
  mSrcVidInfo = std::make_shared<EssenceInfo>(srcTags); 
  printDebug(eInfo, "Stamper SrcVidInfo: %s\n", mSrcVidInfo->toString().c_str());
  mDstVidInfo = std::make_shared<EssenceInfo>(dstTags); 
  printDebug(eInfo, "Stamper DstVidInfo: %s\n", mDstVidInfo->toString().c_str());

  if (mSrcVidInfo->packing().compare(mDstVidInfo->packing())) {
    std::string err = std::string("Source and destination format must be identical \'") + mSrcVidInfo->packing() + "\', \'" + mDstVidInfo->packing() + "\'";
    return Nan::ThrowError(err.c_str());
  }
  if (mSrcVidInfo->packing().compare("420P") && mSrcVidInfo->packing().compare("YUV422P10")) {
    std::string err = std::string("Unsupported source format \'") + mSrcVidInfo->packing() + "\'";
    return Nan::ThrowError(err.c_str());
  }
  if (mDstVidInfo->packing().compare("420P") && mDstVidInfo->packing().compare("YUV422P10")) { 
    std::string err = std::string("Unsupported destination packing type \'") + mDstVidInfo->packing() + "\'";
    Nan::ThrowError(err.c_str());
  }
  if ((mSrcVidInfo->width() % 2) || (mDstVidInfo->width() % 2)) {
    std::string err = std::string("Width must be divisible by 2 - src ") + std::to_string(mSrcVidInfo->width()) + ", dst " + std::to_string(mDstVidInfo->width());
    Nan::ThrowError(err.c_str());
  }

  mDstBytesReq = getFormatBytes(mDstVidInfo->packing(), mDstVidInfo->width(), mDstVidInfo->height());
}

void Stamper::doWipe(std::shared_ptr<WipeProcessData> wpd) {
  uint32_t blackLevel = 64;
  uint32_t lumaRange = 940 - blackLevel;
  uint32_t chromaRange = 960 - blackLevel;
  uint32_t chromaMid = 512;
  uint32_t bytesPerPixel = 2;
  uint32_t lumaLinesPerChromaLine = 1;
  if (0 == mSrcVidInfo->packing().compare("420P")) {
    blackLevel = 16;
    lumaRange = 235 - blackLevel;
    chromaRange = 240 - blackLevel;
    chromaMid = 128;
    bytesPerPixel = 1;
    lumaLinesPerChromaLine = 2;
  }

  iCol wipeCol (uint16_t(wpd->wipeCol().y * lumaRange + blackLevel),
                uint16_t(wpd->wipeCol().u * chromaRange + chromaMid),  
                uint16_t(wpd->wipeCol().v * chromaRange + chromaMid));

  uint32_t dstLumaPitchBytes = mDstVidInfo->width() * bytesPerPixel;
  uint32_t dstChromaPitchBytes = dstLumaPitchBytes / 2;
  uint32_t dstLumaPlaneBytes = dstLumaPitchBytes * mDstVidInfo->height();
  uint32_t dstChromaPlaneBytes = dstChromaPitchBytes * mDstVidInfo->height() / lumaLinesPerChromaLine;

  uint8_t *dstYLine = wpd->dstBuf()->buf();
  uint8_t *dstULine = dstYLine + dstLumaPlaneBytes;
  uint8_t *dstVLine = dstULine + dstChromaPlaneBytes;

  dstYLine += wpd->wipeRect().org.x * bytesPerPixel + dstLumaPitchBytes * wpd->wipeRect().org.y;
  dstULine += wpd->wipeRect().org.x * bytesPerPixel / 2 + dstChromaPitchBytes * wpd->wipeRect().org.y / lumaLinesPerChromaLine;
  dstVLine += wpd->wipeRect().org.x * bytesPerPixel / 2 + dstChromaPitchBytes * wpd->wipeRect().org.y / lumaLinesPerChromaLine;

  for (int32_t y=0; y<wpd->wipeRect().len.y; ++y) {
    bool evenLine = (y & 1) == 0;

    if (1==bytesPerPixel) {
      // 8-bit fill
      memset (dstYLine, wipeCol.y, wpd->wipeRect().len.x);
      memset (dstULine, wipeCol.u, wpd->wipeRect().len.x / 2);
      memset (dstVLine, wipeCol.v, wpd->wipeRect().len.x / 2);
    } else {
      // 10-bit fill
      uint32_t *dstY32Line = (uint32_t *)dstYLine;
      uint16_t *dstU16Line = (uint16_t *)dstULine;
      uint16_t *dstV16Line = (uint16_t *)dstVLine;

      for (int32_t x=0; x < wpd->wipeRect().len.x / 2; ++x) { 
        *dstY32Line++ = (wipeCol.y << 16) | wipeCol.y;
        *dstU16Line++ = wipeCol.u;
        *dstV16Line++ = wipeCol.v;
      }
    }

    dstYLine += dstLumaPitchBytes; 
    if ((1 == lumaLinesPerChromaLine) || !evenLine) {
      dstULine += dstChromaPitchBytes;
      dstVLine += dstChromaPitchBytes;
    } 
  }
}

void Stamper::doCopy(std::shared_ptr<CopyProcessData> cpd) {
  uint32_t bytesPerPixel = 2;
  uint32_t lumaLinesPerChromaLine = 1;
  if (0 == mSrcVidInfo->packing().compare("420P")) {
    bytesPerPixel = 1;
    lumaLinesPerChromaLine = 2;
  }

  uint32_t srcLumaPitchBytes = mSrcVidInfo->width() * bytesPerPixel;
  uint32_t srcChromaPitchBytes = srcLumaPitchBytes / 2;
  uint32_t srcLumaPlaneBytes = srcLumaPitchBytes * mSrcVidInfo->height();
  uint32_t srcChromaPlaneBytes = srcChromaPitchBytes * mSrcVidInfo->height() / lumaLinesPerChromaLine;

  uint32_t dstLumaPitchBytes = mDstVidInfo->width() * bytesPerPixel;
  uint32_t dstChromaPitchBytes = dstLumaPitchBytes / 2;
  uint32_t dstLumaPlaneBytes = dstLumaPitchBytes * mDstVidInfo->height();
  uint32_t dstChromaPlaneBytes = dstChromaPitchBytes * mDstVidInfo->height() / lumaLinesPerChromaLine;

  const uint8_t *srcYLine = cpd->srcBuf()->buf();
  const uint8_t *srcULine = srcYLine + srcLumaPlaneBytes;
  const uint8_t *srcVLine = srcULine + srcChromaPlaneBytes;

  uint8_t *dstYLine = cpd->dstBuf()->buf();
  uint8_t *dstULine = dstYLine + dstLumaPlaneBytes;
  uint8_t *dstVLine = dstULine + dstChromaPlaneBytes;

  dstYLine += cpd->dstOrg().x * bytesPerPixel + dstLumaPitchBytes * cpd->dstOrg().y;
  dstULine += cpd->dstOrg().x * bytesPerPixel / 2 + dstChromaPitchBytes * cpd->dstOrg().y / lumaLinesPerChromaLine;
  dstVLine += cpd->dstOrg().x * bytesPerPixel / 2 + dstChromaPitchBytes * cpd->dstOrg().y / lumaLinesPerChromaLine;

  for (uint32_t y=0; y<mSrcVidInfo->height(); ++y) {
    bool evenLine = (y & 1) == 0;

    memcpy(dstYLine, srcYLine, srcLumaPitchBytes);
    memcpy(dstULine, srcULine, srcChromaPitchBytes);
    memcpy(dstVLine, srcVLine, srcChromaPitchBytes);

    srcYLine += srcLumaPitchBytes;
    dstYLine += dstLumaPitchBytes; 
    if ((1 == lumaLinesPerChromaLine) || !evenLine) {
      srcULine += srcChromaPitchBytes;
      srcVLine += srcChromaPitchBytes;
      dstULine += dstChromaPitchBytes;
      dstVLine += dstChromaPitchBytes;
    } 
  }
}

void Stamper::doMix(std::shared_ptr<MixProcessData> mpd) {
  uint32_t bytesPerPixel = 2;
  uint32_t lumaLinesPerChromaLine = 1;
  if (0 == mSrcVidInfo->packing().compare("420P")) {
    bytesPerPixel = 1;
    lumaLinesPerChromaLine = 2;
  }

  uint32_t srcLumaPitchBytes = mSrcVidInfo->width() * bytesPerPixel;
  uint32_t srcChromaPitchBytes = srcLumaPitchBytes / 2;
  uint32_t srcLumaPlaneBytes = srcLumaPitchBytes * mSrcVidInfo->height();
  uint32_t srcChromaPlaneBytes = srcChromaPitchBytes * mSrcVidInfo->height() / lumaLinesPerChromaLine;

  uint32_t dstLumaPitchBytes = mDstVidInfo->width() * bytesPerPixel;
  uint32_t dstChromaPitchBytes = dstLumaPitchBytes / 2;
  uint32_t dstLumaPlaneBytes = dstLumaPitchBytes * mDstVidInfo->height();
  uint32_t dstChromaPlaneBytes = dstChromaPitchBytes * mDstVidInfo->height() / lumaLinesPerChromaLine;

  const uint8_t *srcLine[2][3];
  for (uint32_t s=0; s<2; ++s) { 
    srcLine[s][0] = mpd->srcBufs()[s]->buf();
    srcLine[s][1] = srcLine[s][0] + srcLumaPlaneBytes;
    srcLine[s][2] = srcLine[s][1] + srcChromaPlaneBytes;
  }

  uint8_t *dstLine[3];
  dstLine[0] = mpd->dstBuf()->buf();
  dstLine[1] = dstLine[0] + dstLumaPlaneBytes;
  dstLine[2] = dstLine[1] + dstChromaPlaneBytes;
  
  float pressure = mpd->pressure();
        
  for (uint32_t p=0; p<3; ++p) {
    uint32_t numPixels = (0==p) ? mSrcVidInfo->width() : mSrcVidInfo->width() / 2;
    for (uint32_t y=0; y<mSrcVidInfo->height(); ++y) {
      bool evenLine = (y & 1) == 0;
      if (1==bytesPerPixel) {
        const uint8_t *srcA = srcLine[0][p];
        const uint8_t *srcB = srcLine[1][p];
        uint8_t *dst = dstLine[p];

        if ((0==p) || evenLine) {
          for (uint32_t x=0; x < numPixels; ++x)
            *dst++ = uint8_t((float)*srcA++ * pressure + (float)*srcB++ * (1.0f - pressure));
        }
      } else {
        const uint16_t *srcA = (const uint16_t *)srcLine[0][p];
        const uint16_t *srcB = (const uint16_t *)srcLine[1][p];
        uint16_t *dst = (uint16_t *)dstLine[p];

        for (uint32_t x=0; x < numPixels; ++x)
          *dst++ = uint16_t((float)*srcA++ * pressure + (float)*srcB++ * (1.0f - pressure));
      }

      if ((0==p) || (1 == lumaLinesPerChromaLine) || !evenLine) {
        for (uint32_t s=0; s<2; ++s)
          srcLine[s][p] += (0==p) ? srcLumaPitchBytes : srcChromaPitchBytes;
        dstLine[p] += (0==p) ? dstLumaPitchBytes : dstChromaPitchBytes;
      }
    }
  }
}

void Stamper::doStamp(std::shared_ptr<StampProcessData> spd) {
  uint32_t bytesPerPixel = 2;
  uint32_t lumaLinesPerChromaLine = 1;
  if (0 == mSrcVidInfo->packing().compare("420P")) {
    bytesPerPixel = 1;
    lumaLinesPerChromaLine = 2;
  }

  uint32_t srcLumaPitchBytes = mSrcVidInfo->width() * bytesPerPixel;
  uint32_t srcChromaPitchBytes = srcLumaPitchBytes / 2;
  uint32_t srcLumaPlaneBytes = srcLumaPitchBytes * mSrcVidInfo->height();
  uint32_t srcChromaPlaneBytes = srcChromaPitchBytes * mSrcVidInfo->height() / lumaLinesPerChromaLine;

  uint32_t dstLumaPitchBytes = mDstVidInfo->width() * bytesPerPixel;
  uint32_t dstChromaPitchBytes = dstLumaPitchBytes / 2;
  uint32_t dstLumaPlaneBytes = dstLumaPitchBytes * mDstVidInfo->height();
  uint32_t dstChromaPlaneBytes = dstChromaPitchBytes * mDstVidInfo->height() / lumaLinesPerChromaLine;

  const uint8_t *srcLine[2][4];
  for (uint32_t s=0; s<2; ++s) { 
    srcLine[s][0] = spd->srcBufs()[s]->buf();
    srcLine[s][1] = srcLine[s][0] + srcLumaPlaneBytes;
    srcLine[s][2] = srcLine[s][1] + srcChromaPlaneBytes;
    srcLine[s][3] = srcLine[s][2] + srcChromaPlaneBytes;
  }

  uint8_t *dstLine[3];
  dstLine[0] = spd->dstBuf()->buf();
  dstLine[1] = dstLine[0] + dstLumaPlaneBytes;
  dstLine[2] = dstLine[1] + dstChromaPlaneBytes;
  
  for (uint32_t y=0; y<mSrcVidInfo->height(); ++y) {
    bool evenLine = (y & 1) == 0;
    if (1==bytesPerPixel) {
      const uint8_t *srcAY = srcLine[0][0];
      const uint8_t *srcAU = srcLine[0][1];
      const uint8_t *srcAV = srcLine[0][2];
      const uint8_t *srcAA = srcLine[0][3];
      const uint8_t *srcBY = srcLine[1][0];
      const uint8_t *srcBU = srcLine[1][1];
      const uint8_t *srcBV = srcLine[1][2];
      uint8_t *dstY = dstLine[0];
      uint8_t *dstU = dstLine[1];
      uint8_t *dstV = dstLine[2];
      for (uint32_t x=0; x<mSrcVidInfo->width(); x+=2) {
        float p0 = (float)*srcAA++/255.0f;
        float p1 = (float)*srcAA++/255.0f;
        *dstY++ = uint8_t((float)*srcAY++ * p0 + (float)*srcBY++ * (1.0f - p0));
        *dstY++ = uint8_t((float)*srcAY++ * p1 + (float)*srcBY++ * (1.0f - p1));
        if (evenLine) {
          *dstU++ = uint8_t((float)*srcAU++ * p0 + (float)*srcBU++ * (1.0f - p0));
          *dstV++ = uint8_t((float)*srcAV++ * p0 + (float)*srcBV++ * (1.0f - p0));
        }
      }

      for (uint32_t s=0; s<2; ++s) { 
        srcLine[s][0] += srcLumaPitchBytes;
        if (!evenLine) {
          srcLine[s][1] += srcChromaPitchBytes;
          srcLine[s][2] += srcChromaPitchBytes;
        }
        srcLine[s][3] += srcLumaPitchBytes;
      }

      dstLine[0] += dstLumaPitchBytes;
      if (!evenLine) {
        dstLine[1] += dstChromaPitchBytes;
        dstLine[2] += dstChromaPitchBytes;
      }
    } else {
      const uint16_t *srcAY = (const uint16_t *)srcLine[0][0];
      const uint16_t *srcAU = (const uint16_t *)srcLine[0][1];
      const uint16_t *srcAV = (const uint16_t *)srcLine[0][2];
      const uint16_t *srcAA = (const uint16_t *)srcLine[0][3];
      const uint16_t *srcBY = (const uint16_t *)srcLine[1][0];
      const uint16_t *srcBU = (const uint16_t *)srcLine[1][1];
      const uint16_t *srcBV = (const uint16_t *)srcLine[1][2];
      uint16_t *dstY = (uint16_t *)dstLine[0];
      uint16_t *dstU = (uint16_t *)dstLine[1];
      uint16_t *dstV = (uint16_t *)dstLine[2];
      for (uint32_t x=0; x<mSrcVidInfo->width(); x+=2) {
        float p0 = (float)*srcAA++/1023.0f;
        float p1 = (float)*srcAA++/1023.0f;
        *dstY++ = uint16_t((float)*srcAY++ * p0 + (float)*srcBY++ * (1.0f - p0));
        *dstY++ = uint16_t((float)*srcAY++ * p1 + (float)*srcBY++ * (1.0f - p1));
        *dstU++ = uint16_t((float)*srcAU++ * p0 + (float)*srcBU++ * (1.0f - p0));
        *dstV++ = uint16_t((float)*srcAV++ * p0 + (float)*srcBV++ * (1.0f - p0));
      }

      for (uint32_t s=0; s<2; ++s) { 
        srcLine[s][0] += srcLumaPitchBytes;
        srcLine[s][1] += srcChromaPitchBytes;
        srcLine[s][2] += srcChromaPitchBytes;
        srcLine[s][3] += srcLumaPitchBytes;
      }

      dstLine[0] += dstLumaPitchBytes;
      dstLine[1] += dstChromaPitchBytes;
      dstLine[2] += dstChromaPitchBytes;
    }
  }
}

NAN_METHOD(Stamper::SetInfo) {
  if (info.Length() != 3)
    return Nan::ThrowError("Stamper SetInfo expects 3 arguments");
  if (!info[0]->IsArray())
    return Nan::ThrowError("Stamper SetInfo requires a valid srcTags array as the first parameter");
  if (!info[1]->IsObject())
    return Nan::ThrowError("Stamper SetInfo requires a valid dstTags object as the second parameter");
  if (!info[2]->IsNumber())
    return Nan::ThrowError("Stamper SetInfo requires a valid debug level as the third parameter");

  Local<Array> srcTagsArray = Local<Array>::Cast(info[0]);
  Local<Object> dstTags = Local<Object>::Cast(info[1]);

  Stamper* obj = Nan::ObjectWrap::Unwrap<Stamper>(info.Holder());
  obj->setDebug((eDebugLevel)Nan::To<uint32_t>(info[2]).FromJust());
  
  Nan::TryCatch try_catch;
  obj->doSetInfo(srcTagsArray, dstTags);
  if (try_catch.HasCaught()) {
    obj->mSetInfoOK = false;
    try_catch.ReThrow();
    return;
  }

  obj->mSetInfoOK = true;
  info.GetReturnValue().Set(Nan::New(obj->mDstBytesReq));
}

NAN_METHOD(Stamper::Wipe) {
  if (info.Length() != 3)
    return Nan::ThrowError("Stamper Wipe expects 3 arguments");
  if (!info[0]->IsObject())
    return Nan::ThrowError("Stamper Wipe requires a valid destination buffer as the first parameter");
  if (!info[1]->IsObject())
    return Nan::ThrowError("Stamper Wipe requires a valid params object as the second parameter");
  if (!info[2]->IsFunction())
    return Nan::ThrowError("Stamper Wipe requires a valid callback as the third parameter");

  Local<Object> dstBufObj = Local<Object>::Cast(info[0]);
  Local<Object> paramTags = Local<Object>::Cast(info[1]);
  Local<Function> callback = Local<Function>::Cast(info[2]);

  Stamper* obj = Nan::ObjectWrap::Unwrap<Stamper>(info.Holder());

  if (!obj->mSetInfoOK)
    return Nan::ThrowError("Wipe called with incorrect setup parameters");

  if (obj->mDstBytesReq > node::Buffer::Length(dstBufObj))
    return Nan::ThrowError("Insufficient destination buffer for specified format");

  Local<String> wipeRectStr = Nan::New<String>("wipeRect").ToLocalChecked();
  Local<Array> wipeRectArr = Local<Array>::Cast(Nan::Get(paramTags, wipeRectStr).ToLocalChecked());
  if (!(!wipeRectArr->IsNull() && wipeRectArr->IsArray() && (wipeRectArr->Length() == 4)))
    return Nan::ThrowError("wipeRect parameter invalid");
  iRect wipeRect(iXY(Nan::To<uint32_t>(wipeRectArr->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), 0).ToLocalChecked()).FromJust(), Nan::To<uint32_t>(wipeRectArr->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), 1).ToLocalChecked()).FromJust()),
                 iXY(Nan::To<uint32_t>(wipeRectArr->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), 2).ToLocalChecked()).FromJust(), Nan::To<uint32_t>(wipeRectArr->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), 3).ToLocalChecked()).FromJust()));

  Local<String> wipeColStr = Nan::New<String>("wipeCol").ToLocalChecked();
  Local<Array> wipeColArr = Local<Array>::Cast(Nan::Get(paramTags, wipeColStr).ToLocalChecked());
  if (!(!wipeColArr->IsNull() && wipeColArr->IsArray() && (wipeColArr->Length() == 3)))
    return Nan::ThrowError("wipeCol parameter invalid");

  fCol wipeCol(Nan::To<double>(wipeColArr->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), 0).ToLocalChecked()).FromJust(), 
               Nan::To<double>(wipeColArr->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), 1).ToLocalChecked()).FromJust(),
               Nan::To<double>(wipeColArr->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), 2).ToLocalChecked()).FromJust());

  std::shared_ptr<iProcessData> wpd = 
    std::make_shared<WipeProcessData>(dstBufObj, wipeRect, wipeCol);
  obj->mWorker->doFrame(wpd, obj, new Nan::Callback(callback));

  info.GetReturnValue().Set(Nan::New(obj->mWorker->numQueued()));
}

NAN_METHOD(Stamper::Copy) {
  if (info.Length() != 4)
    return Nan::ThrowError("Stamper Copy expects 4 arguments");
  if (!info[0]->IsArray())
    return Nan::ThrowError("Stamper Copy requires a valid source buffer array as the first parameter");
  if (!info[1]->IsObject())
    return Nan::ThrowError("Stamper Copy requires a valid destination buffer as the second parameter");
  if (!info[2]->IsObject())
    return Nan::ThrowError("Stamper Copy requires a valid params object as the third parameter");
  if (!info[3]->IsFunction())
    return Nan::ThrowError("Stamper Copy requires a valid callback as the fourth parameter");

  Local<Array> srcBufArray = Local<Array>::Cast(info[0]);
  Local<Object> dstBufObj = Local<Object>::Cast(info[1]);
  Local<Object> paramTags = Local<Object>::Cast(info[2]);
  Local<Function> callback = Local<Function>::Cast(info[3]);

  Local<Object> srcBufObj = Local<Object>::Cast(srcBufArray->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), 0).ToLocalChecked());

  Stamper* obj = Nan::ObjectWrap::Unwrap<Stamper>(info.Holder());

  if (!obj->mSetInfoOK)
    return Nan::ThrowError("Copy called with incorrect setup parameters");

  uint32_t srcFormatBytes = getFormatBytes(obj->mSrcVidInfo->packing(), obj->mSrcVidInfo->width(), obj->mSrcVidInfo->height());
  if (srcFormatBytes > (uint32_t)node::Buffer::Length(srcBufObj))
    Nan::ThrowError("Insufficient source buffer for Copy\n");

  if (obj->mDstBytesReq > node::Buffer::Length(dstBufObj))
    return Nan::ThrowError("Insufficient destination buffer for specified format");

  Local<String> dstOrgStr = Nan::New<String>("dstOrg").ToLocalChecked();
  Local<Array> dstOrgXY = Local<Array>::Cast(Nan::Get(paramTags, dstOrgStr).ToLocalChecked());
  if (!(!dstOrgXY->IsNull() && dstOrgXY->IsArray() && (dstOrgXY->Length() == 2)))
    return Nan::ThrowError("dstOrg parameter invalid");
  iXY dstOrg(Nan::To<uint32_t>(dstOrgXY->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), 0).ToLocalChecked()).FromJust(), Nan::To<uint32_t>(dstOrgXY->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), 1).ToLocalChecked()).FromJust());

  std::shared_ptr<iProcessData> cpd = 
    std::make_shared<CopyProcessData>(srcBufObj, dstBufObj, dstOrg);
  obj->mWorker->doFrame(cpd, obj, new Nan::Callback(callback));

  info.GetReturnValue().Set(Nan::New(obj->mWorker->numQueued()));
}

NAN_METHOD(Stamper::Mix) {
  if (info.Length() != 4)
    return Nan::ThrowError("Stamper Mix expects 4 arguments");
  if (!info[0]->IsArray())
    return Nan::ThrowError("Stamper Mix requires a valid source buffer array as the first parameter");
  if (!info[1]->IsObject())
    return Nan::ThrowError("Stamper Mix requires a valid destination buffer as the second parameter");
  if (!info[2]->IsObject())
    return Nan::ThrowError("Stamper Mix requires a valid params object as the third parameter");
  if (!info[3]->IsFunction())
    return Nan::ThrowError("Stamper Mix requires a valid callback as the fourth parameter");

  Local<Array> srcBufArray = Local<Array>::Cast(info[0]);
  Local<Object> dstBufObj = Local<Object>::Cast(info[1]);
  Local<Object> paramTags = Local<Object>::Cast(info[2]);
  Local<Function> callback = Local<Function>::Cast(info[3]);

  Stamper* obj = Nan::ObjectWrap::Unwrap<Stamper>(info.Holder());

  if (!obj->mSetInfoOK)
    return Nan::ThrowError("Mix called with incorrect setup parameters");

  uint32_t srcFormatBytes = getFormatBytes(obj->mSrcVidInfo->packing(), obj->mSrcVidInfo->width(), obj->mSrcVidInfo->height());
  for (uint32_t i=0; i<srcBufArray->Length(); ++i) {
    Local<Object> srcBufObj = Local<Object>::Cast(srcBufArray->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), i).ToLocalChecked());
    if (srcFormatBytes > (uint32_t)node::Buffer::Length(srcBufObj))
      Nan::ThrowError("Insufficient source buffer for Mix\n");
  }

  if (obj->mDstBytesReq > node::Buffer::Length(dstBufObj))
    return Nan::ThrowError("Insufficient destination buffer for specified format");

  Local<String> pressureStr = Nan::New<String>("pressure").ToLocalChecked();
  Local<Object> pressureObj = Local<Object>::Cast(Nan::Get(paramTags, pressureStr).ToLocalChecked());
  if (pressureObj->IsNull())
    return Nan::ThrowError("pressure parameter invalid");
  float pressure = (float)Nan::To<double>(pressureObj).FromJust();

  std::shared_ptr<iProcessData> mpd = 
    std::make_shared<MixProcessData>(srcBufArray, dstBufObj, pressure);
  obj->mWorker->doFrame(mpd, obj, new Nan::Callback(callback));

  info.GetReturnValue().Set(Nan::New(obj->mWorker->numQueued()));
}

NAN_METHOD(Stamper::Stamp) {
  if (info.Length() != 4)
    return Nan::ThrowError("Stamper Stamp expects 4 arguments");
  if (!info[0]->IsArray())
    return Nan::ThrowError("Stamper Stamp requires a valid source buffer array as the first parameter");
  if (!info[1]->IsObject())
    return Nan::ThrowError("Stamper Stamp requires a valid destination buffer as the second parameter");
  if (!info[2]->IsObject())
    return Nan::ThrowError("Stamper Stamp requires a valid params object as the third parameter");
  if (!info[3]->IsFunction())
    return Nan::ThrowError("Stamper Stamp requires a valid callback as the fourth parameter");

  Local<Array> srcBufArray = Local<Array>::Cast(info[0]);
  Local<Object> dstBufObj = Local<Object>::Cast(info[1]);
  // Local<Object> paramTags = Local<Object>::Cast(info[2]);
  Local<Function> callback = Local<Function>::Cast(info[3]);

  Stamper* obj = Nan::ObjectWrap::Unwrap<Stamper>(info.Holder());

  if (!obj->mSetInfoOK)
    return Nan::ThrowError("Stamp called with incorrect setup parameters");

  if (!obj->mSrcVidInfo->hasAlpha())
    return Nan::ThrowError("Stamp called with source buffer having no alpha channel");

  for (uint32_t i=0; i<srcBufArray->Length(); ++i) {
    uint32_t srcFormatBytes = getFormatBytes(obj->mSrcVidInfo->packing(), obj->mSrcVidInfo->width(), obj->mSrcVidInfo->height(), 0==i);
    Local<Object> srcBufObj = Local<Object>::Cast(srcBufArray->Get(v8::Isolate::GetCurrent()->GetCurrentContext(), i).ToLocalChecked());
    if (srcFormatBytes > (uint32_t)node::Buffer::Length(srcBufObj))
      Nan::ThrowError("Insufficient source buffer for Stamp\n");
  }

  if (obj->mDstBytesReq > node::Buffer::Length(dstBufObj))
    return Nan::ThrowError("Insufficient destination buffer for specified format");

  std::shared_ptr<iProcessData> spd = 
    std::make_shared<StampProcessData>(srcBufArray, dstBufObj);
  obj->mWorker->doFrame(spd, obj, new Nan::Callback(callback));

  info.GetReturnValue().Set(Nan::New(obj->mWorker->numQueued()));
}

NAN_METHOD(Stamper::Quit) {
  if (info.Length() != 1)
    return Nan::ThrowError("Packer quit expects 1 argument");
  if (!info[0]->IsFunction())
    return Nan::ThrowError("Packer quit requires a valid callback as the parameter");
  Nan::Callback *callback = new Nan::Callback(Local<Function>::Cast(info[0]));
  Stamper* obj = Nan::ObjectWrap::Unwrap<Stamper>(info.Holder());

  if (obj->mWorker != NULL)
    obj->mWorker->quit(callback);

  info.GetReturnValue().SetUndefined();
}

NAN_MODULE_INIT(Stamper::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Copy").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(tpl, "setInfo", SetInfo);
  SetPrototypeMethod(tpl, "wipe", Wipe);
  SetPrototypeMethod(tpl, "copy", Copy);
  SetPrototypeMethod(tpl, "mix", Mix);
  SetPrototypeMethod(tpl, "stamp", Stamp);
  SetPrototypeMethod(tpl, "quit", Quit);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("Stamper").ToLocalChecked(),
    Nan::GetFunction(tpl).ToLocalChecked());
}

} // namespace streampunk
