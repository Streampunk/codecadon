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
#include "Packers.h"
#include "Memory.h"

// V210: https://developer.apple.com/library/mac/technotes/tn2162/_index.html#//apple_ref/doc/uid/DTS40013070-CH1-TNTAG8-V210__4_2_2_COMPRESSION_TYPE
// 420P: https://en.wikipedia.org/wiki/YUV
// Pgroup/RFC4175 YUV 422 - big-endian fully packed, 5 bytes for 2 pixels:
//         s0       s1       s2       s3       s4
// msb |--------|--------|--------|--------|--------| lsb
//          u0         y0         v0         y1
// msb |----------|----------|----------|----------| lsb


namespace streampunk {

uint32_t getFormatBytes(const std::string& fmtCode, uint32_t width, uint32_t height) {
  uint32_t fmtBytes = 0;
  if (0 == fmtCode.compare("420P")) {
    fmtBytes = width * height * 3 / 2;
  }
  else if (0 == fmtCode.compare("pgroup")) {
    uint32_t pitchBytes = width * 5 / 2;
    fmtBytes = pitchBytes * height;
  }
  else if (0 == fmtCode.compare("v210")) {
    uint32_t pitchBytes = ((width + 47) / 48) * 48 * 8 / 3;
    fmtBytes = pitchBytes * height;
  }
  else if (0 == fmtCode.compare("UYVY10")) {
    uint32_t pitchBytes = width * 4;
    fmtBytes = pitchBytes * height;
  }
  else if (0 == fmtCode.compare("YUV422P10")) {
    uint32_t pitchBytes = width * 4;
    fmtBytes = pitchBytes * height;
  }
  else if (0 == fmtCode.compare("RGBA8")) {
    uint32_t pitchBytes = width * 4;
    fmtBytes = pitchBytes * height;
  }
  else if ((0 == fmtCode.compare("BGR10-A")) || (0 == fmtCode.compare("BGR10-A-BS"))) {
    fmtBytes = width * height * 4;
  }
  else if (0 == fmtCode.compare("GBRP16")) {
    fmtBytes = width * height * 6;
  }
  else {
    std::string err = std::string("Unsupported format \'") + fmtCode.c_str() + "\'\n";
    Nan::ThrowError(err.c_str());
  }

  return fmtBytes;
}

void dumpPGroupRaw (const uint8_t *const pgbuf, uint32_t width, uint32_t numLines) {
  const uint8_t *buf = pgbuf;
  for (uint32_t i = 0; i < numLines; ++i) {
    printf("PGroup Line%02d: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n", 
      i, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]);
    buf += width * 5 / 2;
  }
}

void dump420P (const uint8_t *const buf, uint32_t width, uint32_t height, uint32_t numLines) {
  const uint8_t *ybuf = buf;
  const uint8_t *ubuf = ybuf + width * height;
  const uint8_t *vbuf = ubuf + width * height / 4;
  for (uint32_t i = 0; i < numLines; ++i) {
    printf("420P Line %02d: Y: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n", 
      i, ybuf[0], ybuf[1], ybuf[2], ybuf[3], ybuf[4], ybuf[5], ybuf[6], ybuf[7]);
    ybuf += width;
    if ((i & 1) == 0) {
      printf("420P Line %02d: U: %02x,     %02x,     %02x,     %02x\n", 
        i, ubuf[0], ubuf[1], ubuf[2], ubuf[3]);
      printf("420P Line %02d: V: %02x,     %02x,     %02x,     %02x\n", 
        i, vbuf[0], vbuf[1], vbuf[2], vbuf[3]);
      ubuf += width / 2;
      vbuf += width / 2;
    }
  }  
}

void dumpBGR10Raw (const uint8_t *const rgbBuf, uint32_t width, uint32_t numLines) {
  const uint32_t *buf = (uint32_t *)rgbBuf;
  for (uint32_t i = 0; i < numLines; ++i) {
    printf("BGR Line%02d: %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x\n", 
      i, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]);
    buf += width;
  }
}

void dumpGBRP16 (const uint8_t *const rgbBuf, uint32_t width, uint32_t height, uint32_t numLines) {
  const uint16_t *gbuf = (uint16_t *)rgbBuf;
  const uint16_t *bbuf = gbuf + width * height;
  const uint16_t *rbuf = bbuf + width * height;
  for (uint32_t i = 0; i < numLines; ++i) {
    printf("G Line%02d: %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x\n", 
      i, gbuf[0], gbuf[1], gbuf[2], gbuf[3], gbuf[4], gbuf[5], gbuf[6], gbuf[7], gbuf[8], gbuf[9]);
    printf("B Line%02d: %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x\n", 
      i, bbuf[0], bbuf[1], bbuf[2], bbuf[3], bbuf[4], bbuf[5], bbuf[6], bbuf[7], bbuf[8], bbuf[9]);
    printf("R Line%02d: %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x\n", 
      i, rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5], rbuf[6], rbuf[7], rbuf[8], rbuf[9]);
    gbuf += width;
    bbuf += width;
    rbuf += width;
  }
}

Packers::Packers(uint32_t srcWidth, uint32_t srcHeight, const std::string& srcFmtCode, const std::string& dstFmtCode)
  : mSrcWidth(srcWidth), mSrcHeight(srcHeight), mSrcFmtCode(srcFmtCode), mDstFmtCode(dstFmtCode), mConvertFn(&Packers::convertNotSupported) {

  if (0 == mDstFmtCode.compare("UYVY10")) {
    if (0 == mSrcFmtCode.compare("YUV422P10"))
      mConvertFn = &Packers::convertYUV422P10toUYVY10;
    else if (0 == mSrcFmtCode.compare("pgroup"))
      mConvertFn = &Packers::convertPGrouptoUYVY10;
    else {
      std::string err = std::string("Unsupported conversion \'") + mSrcFmtCode.c_str() + "\' -> \'" + mDstFmtCode.c_str() + "\'";
      Nan::ThrowError(err.c_str());
    }
  } else if (0 == mDstFmtCode.compare("YUV422P10")) {
    if (0 == mSrcFmtCode.compare("UYVY10"))
      mConvertFn = &Packers::convertUYVY10toYUV422P10;
    else if (0 == mSrcFmtCode.compare("pgroup"))
      mConvertFn = &Packers::convertPGrouptoYUV422P10;
    else if (0 == mSrcFmtCode.compare("v210"))
      mConvertFn = &Packers::convertV210toYUV422P10;
    else {
      std::string err = std::string("Unsupported conversion \'") + mSrcFmtCode.c_str() + "\' -> \'" + mDstFmtCode.c_str() + "\'";
      Nan::ThrowError(err.c_str());
    }
  } else if (0 == mDstFmtCode.compare("420P")) {
    if (0 == mSrcFmtCode.compare("UYVY10"))
      mConvertFn = &Packers::convertUYVY10to420P;
    else if (0 == mSrcFmtCode.compare("pgroup"))
      mConvertFn = &Packers::convertPGroupto420P;
    else if (0 == mSrcFmtCode.compare("v210"))
      mConvertFn = &Packers::convertV210to420P;
    else {
      std::string err = std::string("Unsupported conversion \'") + mSrcFmtCode.c_str() + "\' -> \'" + mDstFmtCode.c_str() + "\'";
      Nan::ThrowError(err.c_str());
    }
  } else if (0 == mDstFmtCode.compare("pgroup")) {
    if (0 == mSrcFmtCode.compare("UYVY10"))
      mConvertFn = &Packers::convertUYVY10toPGroup;
    else if (0 == mSrcFmtCode.compare("YUV422P10"))
      mConvertFn = &Packers::convertYUV422P10toPGroup;
    else if (0 == mSrcFmtCode.compare("420P"))
      mConvertFn = &Packers::convert420PtoPGroup;
    else if (0 == mSrcFmtCode.compare("v210"))
      mConvertFn = &Packers::convertV210toPGroup;
    else {
      std::string err = std::string("Unsupported conversion \'") + mSrcFmtCode.c_str() + "\' -> \'" + mDstFmtCode.c_str() + "\'";
      Nan::ThrowError(err.c_str());
    }
  } else if (0 == mDstFmtCode.compare("v210")) {
    if (0 == mSrcFmtCode.compare("YUV422P10"))
      mConvertFn = &Packers::convertYUV422P10toV210;
    else if (0 == mSrcFmtCode.compare("420P"))
      mConvertFn = &Packers::convert420PtoV210;
    else if (0 == mSrcFmtCode.compare("pgroup"))
      mConvertFn = &Packers::convertPGrouptoV210;
    else {
      std::string err = std::string("Unsupported conversion \'") + mSrcFmtCode.c_str() + "\' -> \'" + mDstFmtCode.c_str() + "\'";
      Nan::ThrowError(err.c_str());
    }
  } else if (0 == mDstFmtCode.compare("GBRP16")) {
    if ((0 == mSrcFmtCode.compare("BGR10-A")) || (0 == mSrcFmtCode.compare("BGR10-A-BS")))
      mConvertFn = &Packers::convertBGR10AtoGBRP16;
    else {
      std::string err = std::string("Unsupported conversion \'") + mSrcFmtCode.c_str() + "\' -> \'" + mDstFmtCode.c_str() + "\'";
      Nan::ThrowError(err.c_str());
    }
  } else {
    std::string err = std::string("Unsupported destination packing format \'") + mDstFmtCode.c_str() + "\'";
    Nan::ThrowError(err.c_str());
  }
}

void Packers::convert(std::shared_ptr<Memory> srcBuf, std::shared_ptr<Memory> dstBuf) const {
  mConvertFn(*this, srcBuf->buf(), dstBuf->buf());
}

// private
void Packers::convertYUV422P10toUYVY10 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {
  uint32_t srcLumaPitchBytes = mSrcWidth * 2;
  uint32_t srcChromaPitchBytes = mSrcWidth;
  uint32_t srcLumaPlaneBytes = srcLumaPitchBytes * mSrcHeight;
  uint32_t dstPitchBytes = mSrcWidth * 4;

  const uint8_t *srcYLine = srcBuf;
  const uint8_t *srcULine = srcBuf + srcLumaPlaneBytes;
  const uint8_t *srcVLine = srcBuf + srcLumaPlaneBytes + srcLumaPlaneBytes / 2;
  uint8_t *dstLine = dstBuf;

  for (uint32_t y=0; y<mSrcHeight; ++y) {
    const uint32_t *srcYInts = (uint32_t *)srcYLine;
    const uint32_t *srcUInts = (uint32_t *)srcULine;
    const uint32_t *srcVInts = (uint32_t *)srcVLine;
    uint32_t *dstInts = (uint32_t *)dstLine;

    for (uint32_t x=0; x<mSrcWidth; x+=4) {
      uint32_t y01 = srcYInts[0];
      uint32_t y23 = srcYInts[1];
      uint32_t u01 = srcUInts[0];
      uint32_t v01 = srcVInts[0];
      srcYInts += 2;
      srcUInts += 1;
      srcVInts += 1;

      dstInts[0] = (u01 & 0xffff) | ((y01 << 16) & 0xffff0000); // u0 | y0
      dstInts[1] = (v01 & 0xffff) | (y01 & 0xffff0000); // v0 | y1
      dstInts[2] = ((u01 >> 16) & 0xffff) | ((y23 << 16) & 0xffff0000); // u1 | y2
      dstInts[3] = ((v01 >> 16) & 0xffff) | (y23 & 0xffff0000); // v1 | y3
      dstInts += 4;
    }

    srcYLine += srcLumaPitchBytes;
    srcULine += srcChromaPitchBytes;
    srcVLine += srcChromaPitchBytes;
    dstLine += dstPitchBytes;
  }  
}

void Packers::convertPGrouptoUYVY10 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {
  uint32_t srcPitchBytes = mSrcWidth * 5 / 2;
  uint32_t dstPitchBytes = mSrcWidth * 4;

  const uint8_t *srcLine = srcBuf;
  uint8_t *dstLine = dstBuf;

  for (uint32_t y=0; y<mSrcHeight; ++y) {
    const uint8_t *srcBytes = srcLine;
    uint32_t *dstInts = (uint32_t *)dstLine;

    for (uint32_t x=0; x<mSrcWidth; x+=2) {
      uint8_t s0 = srcBytes[0];
      uint8_t s1 = srcBytes[1];
      uint8_t s2 = srcBytes[2];
      uint8_t s3 = srcBytes[3];
      uint8_t s4 = srcBytes[4];
      srcBytes += 5;

      dstInts[0] = ((s0 << 2) | ((s1 & 0xc0) >> 6)) | (((s1 & 0x3f) << 20) | ((s2 & 0xf0) << 12)); // u0 | y0
      dstInts[1] = (((s2 & 0x0f) << 6) | ((s3 & 0xfc >> 2))) | (((s3 & 0x03) << 24) | (s4 << 16)); // v0 | y1
      dstInts += 2;
    }

    srcLine += srcPitchBytes;
    dstLine += dstPitchBytes;
  }  
}

void Packers::convertPGrouptoYUV422P10 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {
  uint32_t srcPitchBytes = mSrcWidth * 5 / 2;
  uint32_t dstLumaPitchBytes = mSrcWidth * 2;
  uint32_t dstChromaPitchBytes = mSrcWidth;
  uint32_t dstLumaPlaneBytes = dstLumaPitchBytes * mSrcHeight;

  const uint8_t *srcLine = srcBuf;
  uint8_t *dstYLine = dstBuf;
  uint8_t *dstULine = dstBuf + dstLumaPlaneBytes;
  uint8_t *dstVLine = dstBuf + dstLumaPlaneBytes + dstLumaPlaneBytes / 2;

  for (uint32_t y=0; y<mSrcHeight; ++y) {
    const uint8_t *srcBytes = srcLine;
    uint16_t *dstYShorts = (uint16_t *)dstYLine;
    uint16_t *dstUShorts = (uint16_t *)dstULine;
    uint16_t *dstVShorts = (uint16_t *)dstVLine;

    // read 5 source bytes / 2 source pixels at a time
    for (uint32_t x=0; x<mSrcWidth; x+=2) {
      uint8_t s0 = srcBytes[0];
      uint8_t s1 = srcBytes[1];
      uint8_t s2 = srcBytes[2];
      uint8_t s3 = srcBytes[3];
      uint8_t s4 = srcBytes[4];
      srcBytes += 5;

      dstYShorts[0] = ((s1 & 0x3f) << 4) | ((s2 & 0xf0) >> 4);
      dstYShorts[1] = ((s3 & 0x03) << 8) | s4;
      dstYShorts += 2;

      dstUShorts[0] = (s0 << 2) | ((s1 & 0xc0) >> 6);
      dstUShorts += 1;

      dstVShorts[0] = ((s2 & 0x0f) << 6) | ((s3 & 0xfc >> 2));
      dstVShorts += 1;
    }

    srcLine += srcPitchBytes;
    dstYLine += dstLumaPitchBytes;
    dstULine += dstChromaPitchBytes;
    dstVLine += dstChromaPitchBytes;
  }
}

void Packers::convertV210toYUV422P10 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {
  uint32_t srcPitchBytes = ((mSrcWidth + 47) / 48) * 48 * 8 / 3;
  uint32_t dstLumaPitchBytes = mSrcWidth * 2;
  uint32_t dstChromaPitchBytes = mSrcWidth;
  uint32_t dstLumaPlaneBytes = dstLumaPitchBytes * mSrcHeight;

  const uint8_t *srcLine = srcBuf;
  uint8_t *dstYLine = dstBuf;
  uint8_t *dstULine = dstBuf + dstLumaPlaneBytes;
  uint8_t *dstVLine = dstBuf + dstLumaPlaneBytes + dstLumaPlaneBytes / 2;

  for (uint32_t y=0; y<mSrcHeight; ++y) {
    uint32_t *srcInts = (uint32_t *)srcLine;
    uint32_t *dstYInts = (uint32_t *)dstYLine;
    uint16_t *dstUShorts = (uint16_t *)dstULine;
    uint16_t *dstVShorts = (uint16_t *)dstVLine;

    // read 4 source 32-bit ints / 6 source pixels at a time
    for (uint32_t x=0; x<mSrcWidth/6; ++x) {
      uint32_t s0 = srcInts[0];
      uint32_t s1 = srcInts[1];
      uint32_t s2 = srcInts[2];
      uint32_t s3 = srcInts[3];
      srcInts += 4;

      dstYInts[0] = ((s0 >> 10) & 0x3ff) | ((s1 << 16) & 0x3ff0000); // Y0 | Y1
      dstYInts[1] = ((s1 >> 20) & 0x3ff) | ((s2 << 6) & 0x3ff0000); // Y2 | Y3
      dstYInts[2] = (s3 & 0x3ff) | ((s3 >> 4) & 0x3ff0000); // Y4 | Y5
      dstYInts += 3;

      dstUShorts[0] = (s0 & 0x3ff); // Cb0
      dstUShorts[1] = ((s1 >> 10) & 0x3ff); // Cb1
      dstUShorts[2] = ((s2 >> 20) & 0x3ff); // Cb2
      dstUShorts += 3;

      dstVShorts[0] = ((s0 >> 20) & 0x3ff); // Cr0
      dstVShorts[1] = (s2 & 0x3ff); // Cr1
      dstVShorts[2] = ((s3 >> 10) & 0x3ff); // Cr2
      dstVShorts += 3;
    }

    uint32_t remain = mSrcWidth%6;
    if (remain) {
      uint32_t s0 = srcInts[0];
      uint32_t s1 = srcInts[1];
      uint64_t s2 = 0;
      if (4==remain)
        s2 = srcInts[2];         

      dstYInts[0] = ((s0 >> 10) & 0x3ff) | ((s1 << 16) & 0x3ff0000); // Y0 | Y1
      if (4==remain)
        dstYInts[1] = ((s1 >> 20) & 0x3ff) | ((s2 << 6) & 0x3ff0000); // Y2 | Y3

      dstUShorts[0] = (s0 & 0x3ff); // Cb0
      if (4==remain)
        dstUShorts[1] = ((s1 >> 10) & 0x3ff); // Cb1

      dstVShorts[0] = ((s0 >> 20) & 0x3ff); // Cr0
      if (4==remain)
        dstVShorts[1] = (s2 & 0x3ff); // Cr1
    }

    srcLine += srcPitchBytes;
    dstYLine += dstLumaPitchBytes;
    dstULine += dstChromaPitchBytes;
    dstVLine += dstChromaPitchBytes;
  }
}

void Packers::convertPGroupto420P (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {
  uint32_t srcPitchBytes = mSrcWidth * 5 / 2;
  uint32_t dstLumaPitchBytes = mSrcWidth;
  uint32_t dstChromaPitchBytes = mSrcWidth / 2;
  uint32_t dstLumaPlaneBytes = mSrcWidth * mSrcHeight;

  const uint8_t *srcLine = srcBuf;
  uint8_t *dstYLine = dstBuf;
  uint8_t *dstULine = dstBuf + dstLumaPlaneBytes;
  uint8_t *dstVLine = dstBuf + dstLumaPlaneBytes + dstLumaPlaneBytes / 4;

  for (uint32_t y=0; y<mSrcHeight; ++y) {
    const uint8_t *srcBytes = srcLine;
    uint8_t *dstYBytes = dstYLine;
    uint8_t *dstUBytes = dstULine;
    uint8_t *dstVBytes = dstVLine;
    bool evenLine = (y & 1) == 0;

    // read 5 source bytes / 2 source pixels at a time
    for (uint32_t x=0; x<mSrcWidth; x+=2) {
      uint8_t s0 = srcBytes[0];
      uint8_t s1 = srcBytes[1];
      uint8_t s2 = srcBytes[2];
      uint8_t s3 = srcBytes[3];
      uint8_t s4 = srcBytes[4];
      srcBytes += 5;

      dstYBytes[0] = ((s1 & 0x3f) << 2) | ((s2 & 0xc0) >> 6);      
      dstYBytes[1] = ((s3 & 0x03) << 6) | ((s4 & 0xfc) >> 2);      
      dstYBytes += 2;

      // chroma interp between lines
      dstUBytes[0] = evenLine ? s0 : (s0 + dstUBytes[0]) >> 1;
      dstUBytes += 1;
      uint32_t v0 = ((s2 & 0x0f) << 4) | ((s3 & 0xf0) >> 4);
      dstVBytes[0] = evenLine ? v0 : (v0 + dstVBytes[0]) >> 1;
      dstVBytes += 1;
    }

    srcLine += srcPitchBytes;
    dstYLine += dstLumaPitchBytes; 
    if (!evenLine) {
      dstULine += dstChromaPitchBytes;
      dstVLine += dstChromaPitchBytes;
    } 
  }
}

void Packers::convertV210to420P (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {
  uint32_t srcPitchBytes = ((mSrcWidth + 47) / 48) * 48 * 8 / 3;
  uint32_t dstLumaPitchBytes = mSrcWidth;
  uint32_t dstChromaPitchBytes = mSrcWidth / 2;
  uint32_t dstLumaPlaneBytes = mSrcWidth * mSrcHeight;

  const uint8_t *srcLine = srcBuf;
  uint8_t *dstYLine = dstBuf;
  uint8_t *dstULine = dstBuf + dstLumaPlaneBytes;
  uint8_t *dstVLine = dstBuf + dstLumaPlaneBytes + dstLumaPlaneBytes / 4;

  for (uint32_t y=0; y<mSrcHeight; ++y) {
    uint32_t *srcInts = (uint32_t *)srcLine;
    uint8_t *dstYBytes = dstYLine;
    uint8_t *dstUBytes = dstULine;
    uint8_t *dstVBytes = dstVLine;
    bool evenLine = (y & 1) == 0;

    // read 4 source ints / 6 source pixels at a time
    for (uint32_t x=0; x<mSrcWidth/6; ++x) {
      uint32_t s0 = srcInts[0];
      uint32_t s1 = srcInts[1];
      uint32_t s2 = srcInts[2];
      uint32_t s3 = srcInts[3];
      srcInts += 4;

      dstYBytes[0] = (s0 >> 12) & 0xff;
      dstYBytes[1] = (s1 >>  2) & 0xff;
      dstYBytes[2] = (s1 >> 22) & 0xff;
      dstYBytes[3] = (s2 >> 12) & 0xff;
      dstYBytes[4] = (s3 >>  2) & 0xff;
      dstYBytes[5] = (s3 >> 22) & 0xff;
      dstYBytes += 6;

      dstUBytes[0] = evenLine ? ((s0 >>  2) & 0xff) : (((s0 >>  2) & 0xff) + dstUBytes[0]) >> 1;
      dstUBytes[1] = evenLine ? ((s1 >> 12) & 0xff) : (((s1 >> 12) & 0xff) + dstUBytes[1]) >> 1;
      dstUBytes[2] = evenLine ? ((s2 >> 22) & 0xff) : (((s2 >> 22) & 0xff) + dstUBytes[2]) >> 1;
      dstUBytes += 3;

      dstVBytes[0] = evenLine ? ((s0 >> 22) & 0xff) : (((s0 >> 22) & 0xff) + dstVBytes[0]) >> 1;
      dstVBytes[1] = evenLine ? ((s2 >>  2) & 0xff) : (((s2 >>  2) & 0xff) + dstVBytes[1]) >> 1;
      dstVBytes[2] = evenLine ? ((s3 >> 12) & 0xff) : (((s3 >> 12) & 0xff) + dstVBytes[2]) >> 1;
      dstVBytes += 3;
    }

    uint32_t remain = mSrcWidth%6;
    if (remain) {
      uint32_t s0 = srcInts[0];
      uint32_t s1 = srcInts[1];
      uint32_t s2 = 0;
      if (4==remain)
        s2 = srcInts[2];

      dstYBytes[0] = (s0 >> 12) & 0xff;
      dstYBytes[1] = (s1 >>  2) & 0xff;
      if (4==remain) {
        dstYBytes[2] = (s1 >> 22) & 0xff;
        dstYBytes[3] = (s2 >> 12) & 0xff;
      }      

      dstUBytes[0] = evenLine ? ((s0 >>  2) & 0xff) : (((s0 >>  2) & 0xff) + dstUBytes[0]) >> 1;
      if (4==remain)
        dstUBytes[1] = evenLine ? ((s1 >> 12) & 0xff) : (((s1 >> 12) & 0xff) + dstUBytes[1]) >> 1;

      dstVBytes[0] = evenLine ? ((s0 >> 22) & 0xff) : (((s0 >> 22) & 0xff) + dstVBytes[0]) >> 1;
      if (4==remain)
        dstVBytes[1] = evenLine ? ((s2 >>  2) & 0xff) : (((s2 >>  2) & 0xff) + dstVBytes[1]) >> 1;
    }

    srcLine += srcPitchBytes;
    dstYLine += dstLumaPitchBytes; 
    if (!evenLine) {
      dstULine += dstChromaPitchBytes;
      dstVLine += dstChromaPitchBytes;
    } 
  }
}

void Packers::convertUYVY10toPGroup (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {
  uint32_t srcPitchBytes = mSrcWidth * 4;
  uint32_t dstPitchBytes = mSrcWidth * 5 / 2;

  const uint8_t *srcLine = srcBuf;
  uint8_t *dstLine = dstBuf;

  for (uint32_t y=0; y<mSrcHeight; ++y) {
    const uint32_t *srcInts = (uint32_t *)srcLine;
    uint8_t *dstBytes = dstLine;

    for (uint32_t x=0; x<mSrcWidth; x+=2) {
      uint32_t s0 = srcInts[0]; // u0 | y0
      uint32_t s1 = srcInts[1]; // v0 | y1
      srcInts += 2;

      dstBytes[0] = (s0 & 0x3fc) >> 2;
      dstBytes[1] = ((s0 & 0x3) << 6) | ((s0 & 0x3f00000) >> 20);
      dstBytes[2] = ((s0 & 0xf0000) >> 12) | ((s1 & 0x3c0) >> 6);
      dstBytes[3] = ((s1 & 0x3f) << 2) | ((s1 & 0x3000000) >> 24);
      dstBytes[4] = (s1 & 0xff0000) >> 16;
      dstBytes += 5;
    }

    srcLine += srcPitchBytes;
    dstLine += dstPitchBytes;
  }  
}

void Packers::convertUYVY10toYUV422P10 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {
  uint32_t srcPitchBytes = mSrcWidth * 4;
  uint32_t dstLumaPitchBytes = mSrcWidth * 2;
  uint32_t dstChromaPitchBytes = mSrcWidth;
  uint32_t dstLumaPlaneBytes = dstLumaPitchBytes * mSrcHeight;

  const uint8_t *srcLine = srcBuf;
  uint8_t *dstYLine = dstBuf;
  uint8_t *dstULine = dstBuf + dstLumaPlaneBytes;
  uint8_t *dstVLine = dstBuf + dstLumaPlaneBytes + dstLumaPlaneBytes / 2;

  for (uint32_t y=0; y<mSrcHeight; ++y) {
    const uint32_t *srcInts = (uint32_t *)srcLine;
    uint32_t *dstYInts = (uint32_t *)dstYLine;
    uint32_t *dstUInts = (uint32_t *)dstULine;
    uint32_t *dstVInts = (uint32_t *)dstVLine;

    for (uint32_t x=0; x<mSrcWidth; x+=4) {
      uint32_t s0 = srcInts[0]; // u0 | y0
      uint32_t s1 = srcInts[1]; // v0 | y1
      uint32_t s2 = srcInts[2]; // u1 | y2
      uint32_t s3 = srcInts[3]; // v1 | y3
      srcInts += 4;

      dstYInts[0] = ((s0 & 0x3ff0000) >> 16) | (s1 & 0x3ff0000);
      dstYInts[1] = ((s2 & 0x3ff0000) >> 16) | (s3 & 0x3ff0000);
      dstUInts[0] = (s0 & 0x3ff) | ((s2 & 0x3ff) << 16);
      dstVInts[0] = (s1 & 0x3ff) | ((s3 & 0x3ff) << 16);
      dstYInts += 2;
      dstUInts += 1;
      dstVInts += 1;
    }

    srcLine += srcPitchBytes;
    dstYLine += dstLumaPitchBytes;
    dstULine += dstChromaPitchBytes;
    dstVLine += dstChromaPitchBytes;
  }  
}

void Packers::convertUYVY10to420P (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {
  uint32_t srcPitchBytes = mSrcWidth * 4;
  uint32_t dstLumaPitchBytes = mSrcWidth;
  uint32_t dstChromaPitchBytes = mSrcWidth / 2;
  uint32_t dstLumaPlaneBytes = dstLumaPitchBytes * mSrcHeight;

  const uint8_t *srcLine = srcBuf;
  uint8_t *dstYLine = dstBuf;
  uint8_t *dstULine = dstBuf + dstLumaPlaneBytes;
  uint8_t *dstVLine = dstBuf + dstLumaPlaneBytes + dstLumaPlaneBytes / 4;

  for (uint32_t y=0; y<mSrcHeight; ++y) {
    const uint32_t *srcInts = (uint32_t *)srcLine;
    uint8_t *dstYBytes = dstYLine;
    uint8_t *dstUBytes = dstULine;
    uint8_t *dstVBytes = dstVLine;
    bool evenLine = (y & 1) == 0;

    for (uint32_t x=0; x<mSrcWidth; x+=2) {
      uint32_t s0 = srcInts[0]; // u0 | y0
      uint32_t s1 = srcInts[1]; // v0 | y1
      srcInts += 2;

      dstYBytes[0] = (s0 & 0x3fc0000) >> 18;
      dstYBytes[1] = (s1 & 0x3fc0000) >> 18;

      dstUBytes[0] = evenLine ? ((s0 & 0x3fc) >> 2) : (((s0 & 0x3fc) >> 2) + dstUBytes[0]) >> 1;
      dstVBytes[0] = evenLine ? ((s1 & 0x3fc) >> 2) : (((s1 & 0x3fc) >> 2) + dstUBytes[0]) >> 1;
      dstYBytes += 2;
      dstUBytes += 1;
      dstVBytes += 1;
    }

    srcLine += srcPitchBytes;
    dstYLine += dstLumaPitchBytes;
    if (!evenLine) {
      dstULine += dstChromaPitchBytes;
      dstVLine += dstChromaPitchBytes;
    } 
  }  
}

void Packers::convertYUV422P10toPGroup (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {
  uint32_t srcLumaPitchBytes = mSrcWidth * 2;
  uint32_t srcChromaPitchBytes = mSrcWidth;
  uint32_t srcLumaPlaneBytes = srcLumaPitchBytes * mSrcHeight;
  uint32_t dstPitchBytes = mSrcWidth * 5 / 2;

  const uint8_t *srcYLine = srcBuf;
  const uint8_t *srcULine = srcBuf + srcLumaPlaneBytes;
  const uint8_t *srcVLine = srcBuf + srcLumaPlaneBytes + srcLumaPlaneBytes / 2;
  uint8_t *dstLine = dstBuf;

  for (uint32_t y=0; y<mSrcHeight; ++y) {
    const uint32_t *srcYInts = (uint32_t *)srcYLine;
    const uint16_t *srcUShorts = (uint16_t *)srcULine;
    const uint16_t *srcVShorts = (uint16_t *)srcVLine;
    uint8_t *dstBytes = (uint8_t *)dstLine;

    for (uint32_t x=0; x<mSrcWidth; x+=2) {
      uint32_t y01 = srcYInts[0];
      uint16_t u0 = srcUShorts[0];
      uint16_t v0 = srcVShorts[0];
      srcYInts++;
      srcUShorts++;
      srcVShorts++;

      dstBytes[0] = ((u0 >> 2) & 0xff);
      dstBytes[1] = ((u0 << 6) & 0xc0) | ((y01 >> 4) & 0x3f);
      dstBytes[2] = ((y01 << 4) & 0xf0) | ((v0 >> 6) & 0x0f);
      dstBytes[3] = ((v0 << 2) & 0xfc) | ((y01 >> 24) & 0x03);
      dstBytes[4] = ((y01 >> 16) & 0xff);
      dstBytes += 5;
    }

    srcYLine += srcLumaPitchBytes;
    srcULine += srcChromaPitchBytes;
    srcVLine += srcChromaPitchBytes;
    dstLine += dstPitchBytes;
  }  
}

void Packers::convert420PtoPGroup (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {
  uint32_t srcLumaPitchBytes = mSrcWidth;
  uint32_t srcChromaPitchBytes = mSrcWidth / 2;
  uint32_t srcLumaPlaneBytes = srcLumaPitchBytes * mSrcHeight;
  uint32_t dstPitchBytes = mSrcWidth * 5 / 2;

  const uint8_t *srcYLine = srcBuf;
  const uint8_t *srcULine = srcBuf + srcLumaPlaneBytes;
  const uint8_t *srcVLine = srcBuf + srcLumaPlaneBytes + srcLumaPlaneBytes / 4;
  uint8_t *dstLine = dstBuf;

  for (uint32_t y=0; y<mSrcHeight; ++y) {
    const uint8_t *srcYBytes = srcYLine;
    const uint8_t *srcUBytes = srcULine;
    const uint8_t *srcVBytes = srcVLine;
    uint8_t *dstBytes = (uint8_t *)dstLine;
    bool evenLine = (y & 1) == 0;

    for (uint32_t x=0; x<mSrcWidth; x+=2) {
      uint8_t y0 = srcYBytes[0];
      uint8_t y1 = srcYBytes[1];
      uint8_t u0 = srcUBytes[0];
      uint8_t v0 = srcVBytes[0];
      srcYBytes++;
      srcUBytes++;
      srcVBytes++;

      dstBytes[0] = u0;
      dstBytes[1] = ((y0 >> 2) & 0x3f);
      dstBytes[2] = ((y0 << 6) & 0xc0) | ((v0 >> 4) & 0x0f);
      dstBytes[3] = ((v0 << 4) & 0xf0) | ((y1 >> 6) & 0x03);
      dstBytes[4] = ((y1 << 2) & 0xff);
      dstBytes += 5;
    }

    srcYLine += srcLumaPitchBytes;
    if (!evenLine) {
      srcULine += srcChromaPitchBytes;
      srcVLine += srcChromaPitchBytes;
    }
    dstLine += dstPitchBytes;
  }  
}

void Packers::convertYUV422P10toV210 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {
  uint32_t srcLumaPitchBytes = mSrcWidth * 2;
  uint32_t srcChromaPitchBytes = mSrcWidth;
  uint32_t srcLumaPlaneBytes = srcLumaPitchBytes * mSrcHeight;
  uint32_t dstPitchBytes = ((mSrcWidth + 47) / 48) * 48 * 8 / 3;

  const uint8_t *srcYLine = srcBuf;
  const uint8_t *srcULine = srcBuf + srcLumaPlaneBytes;
  const uint8_t *srcVLine = srcBuf + srcLumaPlaneBytes + srcLumaPlaneBytes / 2;
  uint8_t *dstLine = dstBuf;

  for (uint32_t y=0; y<mSrcHeight; ++y) {
    const uint32_t *srcYInts = (uint32_t *)srcYLine;
    const uint16_t *srcUShorts = (uint16_t *)srcULine;
    const uint16_t *srcVShorts = (uint16_t *)srcVLine;
    uint32_t *dstInts = (uint32_t *)dstLine;

    for (uint32_t x=0; x<mSrcWidth/6; ++x) {
      uint32_t y01 = srcYInts[0];
      uint32_t y23 = srcYInts[1];
      uint32_t y45 = srcYInts[2];
      uint16_t u0 = srcUShorts[0];
      uint16_t u1 = srcUShorts[1];
      uint16_t u2 = srcUShorts[2];
      uint16_t v0 = srcVShorts[0];
      uint16_t v1 = srcVShorts[1];
      uint16_t v2 = srcVShorts[2];
      srcYInts += 3;
      srcUShorts += 3;
      srcVShorts += 3;

      dstInts[0] = ((v0 << 20) & 0x3ff00000) | ((y01 << 10) & 0xffc00) | (u0 & 0x3ff); // v0 | y0 | u0
      dstInts[1] = ((y23 << 20) & 0x3ff00000) | ((u1 << 10) & 0xffc00) | ((y01 >> 16) & 0x3ff); // y2 | u1 | y1
      dstInts[2] = ((u2 << 20) & 0x3ff00000) | ((y23 >> 6) & 0xffc00) | (v1 & 0x3ff); // u2 | y3 | v1
      dstInts[3] = ((y45 << 4) & 0x3ff00000) | ((v2 << 10) & 0xffc00) | (y45 & 0x3ff); // y5 | v2 | y4
      dstInts += 4;
    }

    uint32_t remain = mSrcWidth%6;
    if (remain) {
      uint32_t y01 = srcYInts[0];
      uint32_t y23 = 0;
      uint16_t u0 = srcUShorts[0];
      uint16_t u1 = 0;
      uint16_t v0 = srcVShorts[0];
      uint16_t v1 = 0;
      if (4==remain) {
        y23 = srcYInts[1];
        u1 = srcUShorts[1];
        v1 = srcVShorts[1];
      }

      dstInts[0] = ((v0 << 20) & 0x3ff00000) | ((y01 << 10) & 0xffc00) | (u0 & 0x3ff); // v0 | y0 | u0
      if (2==remain)
        dstInts[1] = ((y01 >> 16) & 0x3ff); // y1
      else if (4==remain) {
        dstInts[1] = ((y23 << 20) & 0x3ff00000) | ((u1 << 10) & 0xffc00) | ((y01 >> 16) & 0x3ff); // y2 | u1 | y1
        dstInts[2] = ((y23 >> 6) & 0xffc00) | (v1 & 0x3ff); // y3 | v1
      }
    }

    srcYLine += srcLumaPitchBytes;
    srcULine += srcChromaPitchBytes;
    srcVLine += srcChromaPitchBytes;
    dstLine += dstPitchBytes;
  }  
}

void Packers::convert420PtoV210 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {
  uint32_t srcLumaPitchBytes = mSrcWidth;
  uint32_t srcChromaPitchBytes = mSrcWidth / 2;
  uint32_t srcLumaPlaneBytes = srcLumaPitchBytes * mSrcHeight;
  uint32_t dstPitchBytes = ((mSrcWidth + 47) / 48) * 48 * 8 / 3;

  const uint8_t *srcYLine = srcBuf;
  const uint8_t *srcULine = srcBuf + srcLumaPlaneBytes;
  const uint8_t *srcVLine = srcBuf + srcLumaPlaneBytes + srcLumaPlaneBytes / 4;
  uint8_t *dstLine = dstBuf;

  for (uint32_t y=0; y<mSrcHeight; ++y) {
    const uint8_t *srcYBytes = srcYLine;
    const uint8_t *srcUBytes = srcULine;
    const uint8_t *srcVBytes = srcVLine;
    uint32_t *dstInts = (uint32_t *)dstLine;
    bool evenLine = (y & 1) == 0;

    for (uint32_t x=0; x<mSrcWidth/6; ++x) {
      uint8_t y0 = srcYBytes[0];
      uint8_t y1 = srcYBytes[1];
      uint8_t y2 = srcYBytes[2];
      uint8_t y3 = srcYBytes[3];
      uint8_t y4 = srcYBytes[4];
      uint8_t y5 = srcYBytes[5];
      uint8_t u0 = srcUBytes[0];
      uint8_t u1 = srcUBytes[1];
      uint8_t u2 = srcUBytes[2];
      uint8_t v0 = srcVBytes[0];
      uint8_t v1 = srcVBytes[1];
      uint8_t v2 = srcVBytes[2];
      srcYBytes += 6;
      srcUBytes += 3;
      srcVBytes += 3;

      dstInts[0] = ((v0 << 22) & 0x3fc00000) | ((y0 << 12) & 0xff000) | ((u0 << 2) & 0x3fc); // v0 | y0 | u0
      dstInts[1] = ((y2 << 22) & 0x3fc00000) | ((u1 << 12) & 0xff000) | ((y1 << 2) & 0x3fc); // y2 | u1 | y1
      dstInts[2] = ((u2 << 22) & 0x3fc00000) | ((y3 << 12) & 0xff000) | ((v1 << 2) & 0x3fc); // u2 | y3 | v1
      dstInts[3] = ((y5 << 22) & 0x3fc00000) | ((v2 << 12) & 0xff000) | ((y4 << 2) & 0x3fc); // y5 | v2 | y4
      dstInts += 4;
    }

    uint32_t remain = mSrcWidth%6;
    if (remain) {
      uint8_t y0 = srcYBytes[0];
      uint8_t y1 = srcYBytes[1];
      uint8_t y2 = 0;
      uint8_t y3 = 0;
      uint8_t u0 = srcUBytes[0];
      uint8_t u1 = 0;
      uint8_t v0 = srcVBytes[0];
      uint8_t v1 = 0;
      if (4==remain) {
        y2 = srcYBytes[2];
        y3 = srcYBytes[3];
        u1 = srcUBytes[1];
        v1 = srcVBytes[1];
      }

      dstInts[0] = ((v0 << 22) & 0x3fc00000) | ((y0 << 12) & 0xff000) | ((u0 << 2) & 0x3fc); // v0 | y0 | u0
      if (2==remain)
        dstInts[1] = ((y1 << 2) & 0x3fc); // y1
      else if (4==remain) {
        dstInts[1] = ((y2 << 22) & 0x3fc00000) | ((u1 << 12) & 0xff000) | ((y1 << 2) & 0x3fc); // y2 | u1 | y1
        dstInts[2] = ((y3 << 12) & 0xff000) | ((v1 << 2) & 0x3ff); // y3 | v1
      }
    }

    srcYLine += srcLumaPitchBytes;
    if (!evenLine) {
      srcULine += srcChromaPitchBytes;
      srcVLine += srcChromaPitchBytes;
    }
    dstLine += dstPitchBytes;
  }  
}

void Packers::convertPGrouptoV210 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {
  uint32_t srcPitchBytes = mSrcWidth * 5 / 2;
  uint32_t dstPitchBytes = ((mSrcWidth + 47) / 48) * 48 * 8 / 3;

  const uint8_t *srcLine = srcBuf;
  uint8_t *dstLine = dstBuf;

  for (uint32_t y=0; y<mSrcHeight; ++y) {
    const uint8_t *srcBytes = srcLine;
    uint32_t *dstInts = (uint32_t *)dstLine;

    for (uint32_t x=0; x<mSrcWidth/6; ++x) {
      uint8_t s0 = srcBytes[0];
      uint8_t s1 = srcBytes[1];
      uint8_t s2 = srcBytes[2];
      uint8_t s3 = srcBytes[3];
      uint8_t s4 = srcBytes[4];

      uint8_t s5 = srcBytes[5];
      uint8_t s6 = srcBytes[6];
      uint8_t s7 = srcBytes[7];
      uint8_t s8 = srcBytes[8];
      uint8_t s9 = srcBytes[9];

      uint8_t s10 = srcBytes[10];
      uint8_t s11 = srcBytes[11];
      uint8_t s12 = srcBytes[12];
      uint8_t s13 = srcBytes[13];
      uint8_t s14 = srcBytes[14];
      srcBytes += 15;

      dstInts[0] = (((s2 << 26) | (s3 << 18)) & 0x3ff00000) | (((s1 << 14) | (s2 << 6)) & 0xffc00) | (((s0 << 2) | (s1 >> 6)) & 0x3ff); // v0 | y0 | u0
      dstInts[1] = (((s6 << 24) | (s7 << 16)) & 0x3ff00000) | (((s5 << 12) | (s6 << 4)) & 0xffc00) | (((s3 << 8) | s4) & 0x3ff); // y2 | u1 | y1
      dstInts[2] = (((s10 << 22) | (s11 << 14)) & 0x3ff00000) | (((s8 << 18) | (s9 << 10)) & 0xffc00) | (((s7 << 6) | (s8 >> 2)) & 0x3ff); // u2 | y3 | v1
      dstInts[3] = (((s13 << 28) | (s14 << 20)) & 0x3ff00000) | (((s12 << 16) | (s13 << 8)) & 0xffc00) | (((s11 << 4) | (s12 >> 4)) & 0x3ff); // y5 | v2 | y4
      dstInts += 4;
    }

    uint32_t remain = mSrcWidth%6;
    if (remain) {
      uint8_t s0 = srcBytes[0];
      uint8_t s1 = srcBytes[1];
      uint8_t s2 = srcBytes[2];
      uint8_t s3 = srcBytes[3];
      uint8_t s4 = srcBytes[4];

      uint8_t s5 = 0;
      uint8_t s6 = 0;
      uint8_t s7 = 0;
      uint8_t s8 = 0;
      uint8_t s9 = 0;

      if (4==remain) {
        s5 = srcBytes[5];
        s6 = srcBytes[6];
        s7 = srcBytes[7];
        s8 = srcBytes[8];
        s9 = srcBytes[9];
      }

      dstInts[0] = (((s2 << 26) | (s3 << 18)) & 0x3ff00000) | (((s1 << 14) | (s2 << 6)) & 0xffc00) | (((s0 << 2) | (s1 >> 6)) & 0x3ff); // v0 | y0 | u0
      if (2==remain)
        dstInts[1] = (((s3 << 8) | s4) & 0x3ff); // y1
      else if (4==remain) {
        dstInts[1] = (((s6 << 24) | (s7 << 16)) & 0x3ff00000) | (((s5 << 12) | (s6 << 4)) & 0xffc00) | (((s3 << 8) | s4) & 0x3ff); // y2 | u1 | y1
        dstInts[2] = (((s8 << 18) | (s9 << 10)) & 0xffc00) | (((s7 << 6) | (s8 >> 2)) & 0x3ff); // y3 | v1
      }
    }

    srcLine += srcPitchBytes;
    dstLine += dstPitchBytes;
  }
}

void Packers::convertV210toPGroup (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {
  uint32_t srcPitchBytes = ((mSrcWidth + 47) / 48) * 48 * 8 / 3;
  uint32_t dstPitchBytes = mSrcWidth * 5 / 2;

  const uint8_t *srcLine = srcBuf;
  uint8_t *dstLine = dstBuf;

  for (uint32_t y=0; y<mSrcHeight; ++y) {
    const uint32_t *srcInts = (uint32_t *)srcLine;
    uint8_t *dstBytes = dstLine;

    for (uint32_t x=0; x<mSrcWidth/6; ++x) {
      uint32_t s0 = srcInts[0]; // v0 | y0 | u0
      uint32_t s1 = srcInts[1]; // y2 | u1 | y1
      uint32_t s2 = srcInts[2]; // u2 | y3 | v1
      uint32_t s3 = srcInts[3]; // y5 | v2 | y4
      srcInts += 4;

      dstBytes[0] = ((s0 >> 2) & 0xff);
      dstBytes[1] = ((s0 << 6) & 0xc0) | ((s0 >> 14) & 0x3f);
      dstBytes[2] = ((s0 >> 6) & 0xf0) | ((s0 >> 26) & 0x0f);
      dstBytes[3] = ((s0 >> 18) & 0xfc) | ((s1 >> 8) & 0x03);
      dstBytes[4] = (s1 & 0xff);

      dstBytes[5] = ((s1 >> 12) & 0xff);
      dstBytes[6] = ((s1 >> 4) & 0xc0) | ((s1 >> 24) & 0x3f);
      dstBytes[7] = ((s1 >> 16) & 0xf0) | ((s2 >> 6) & 0x0f);
      dstBytes[8] = ((s2 << 2) & 0xfc) | ((s2 >> 18) & 0x03);
      dstBytes[9] = ((s2 >> 10) & 0xff);

      dstBytes[10] = ((s2 >> 22) & 0xff);
      dstBytes[11] = ((s2 >> 14) & 0xc0) | ((s3 >> 4) & 0x3f);
      dstBytes[12] = ((s3 << 4) & 0xf0) | ((s3 >> 16) & 0x0f);
      dstBytes[13] = ((s3 >> 8) & 0xfc) | ((s3 >> 28) & 0x03);
      dstBytes[14] = ((s3 >> 20) & 0xff);
      dstBytes += 15;
    }

    uint32_t remain = mSrcWidth%6;
    if (remain) {
      uint32_t s0 = srcInts[0]; // v0 | y0 | u0
      uint32_t s1 = srcInts[1]; // y2 | u1 | y1
      uint32_t s2 = 0;
      if (4==remain)
        s2 = srcInts[2]; // u2 | y3 | v1

      dstBytes[0] = ((s0 >> 2) & 0xff);
      dstBytes[1] = ((s0 << 6) & 0xc0) | ((s0 >> 14) & 0x3f);
      dstBytes[2] = ((s0 >> 6) & 0xf0) | ((s0 >> 26) & 0x0f);
      dstBytes[3] = ((s0 >> 18) & 0xfc) | ((s1 >> 8) & 0x03);
      dstBytes[4] = (s1 & 0xff);

      if (4==remain) {
        dstBytes[5] = ((s1 >> 12) & 0xff);
        dstBytes[6] = ((s1 >> 4) & 0xc0) | ((s1 >> 24) & 0x3f);
        dstBytes[7] = ((s1 >> 16) & 0xf0) | ((s2 >> 6) & 0x0f);
        dstBytes[8] = ((s2 << 2) & 0xfc) | ((s2 >> 18) & 0x03);
        dstBytes[9] = ((s2 >> 10) & 0xff);
      }
    }

    srcLine += srcPitchBytes;
    dstLine += dstPitchBytes;
  }
}

void Packers::convertBGR10AtoGBRP16 (const uint8_t *const srcBuf, uint8_t *const dstBuf) const {
  bool doByteSwap = (mSrcFmtCode.find("BS") != std::string::npos);
  uint32_t srcPitchBytes = mSrcWidth * 4;
  uint32_t dstPitchBytes = mSrcWidth * 2;
  uint32_t dstPlaneBytes = dstPitchBytes * mSrcHeight;
  
  const uint8_t *srcLine = srcBuf;
  uint8_t *dstGLine = dstBuf;
  uint8_t *dstBLine = dstBuf + dstPlaneBytes;
  uint8_t *dstRLine = dstBuf + dstPlaneBytes * 2;

  for (uint32_t y=0; y<mSrcHeight; ++y) {
    const uint32_t *srcInts = (uint32_t *)srcLine;
    uint16_t *dstGShorts = (uint16_t *)dstGLine;
    uint16_t *dstBShorts = (uint16_t *)dstBLine;
    uint16_t *dstRShorts = (uint16_t *)dstRLine;
    
    if (doByteSwap) {
      for (uint32_t x=0; x<mSrcWidth; ++x) {
        uint32_t s0 = *srcInts++;
        *dstBShorts++ = ((s0 >> 4) & 0xf000) | ((s0 >> 20) & 0x0fc0);
        *dstGShorts++ = ((s0 << 2) & 0xfc00) | ((s0 >> 14) & 0x03f0);
        *dstRShorts++ = ((s0 << 8) & 0xff00) | ((s0 >> 8) & 0x00c0);
      }
    } else {
      for (uint32_t x=0; x<mSrcWidth; ++x) {
        uint32_t s0 = *srcInts++;
        *dstBShorts++ = (s0 << 4) & 0xffc0;
        *dstGShorts++ = (s0 >> 6) & 0xffc0;
        *dstRShorts++ = (s0 >> 16) & 0xffc0;
      }
    }
  
    srcLine += srcPitchBytes;
    dstGLine += dstPitchBytes;
    dstBLine += dstPitchBytes;
    dstRLine += dstPitchBytes;
  }
}

} // namespace streampunk
