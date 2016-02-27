#include <nan.h>
#include "Packers.h"
#include "Memory.h"

namespace streampunk {

void dumpPGroupRaw (uint8_t *pgbuf, uint32_t width, uint32_t numLines) {
  uint8_t *buf = pgbuf;
  for (uint32_t i = 0; i < numLines; ++i) {
    printf("PGroup Line%02d: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n", 
      i, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]);
    buf += width * 5 / 2;
  }
}

void dump420P (uint8_t *buf, uint32_t width, uint32_t height, uint32_t numLines) {
  uint8_t *ybuf = buf;
  uint8_t *ubuf = ybuf + width * height;
  uint8_t *vbuf = ubuf + width * height / 4;
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

Packers::Packers(uint32_t srcWidth, uint32_t srcHeight, const std::string& srcFmtCode, uint32_t srcBytes, const std::string& dstFmtCode)
  : mSrcWidth(srcWidth), mSrcHeight(srcHeight), mSrcFmtCode(srcFmtCode), mSrcBytes(srcBytes), mDstFmtCode(dstFmtCode) {

  if (0 == mSrcFmtCode.compare("4175")) {
    uint32_t srcPitchBytes = mSrcWidth * 5 / 2;
    if (srcPitchBytes * mSrcHeight > mSrcBytes) {
      Nan::ThrowError("insufficient source buffer for conversion\n");
    }
  }
  else if (0 == mSrcFmtCode.compare("v210")) {
    uint32_t srcPitchBytes = ((mSrcWidth + 47) / 48) * 48 * 8 / 3;
    if (srcPitchBytes * mSrcHeight > mSrcBytes) {
      Nan::ThrowError("insufficient source buffer for conversion\n");
    }
  }
  else
    Nan::ThrowError("unsupported source packing type\n");

  if (0 != mDstFmtCode.compare("420P"))
    Nan::ThrowError("unsupported destination packing type\n");
}

std::shared_ptr<Memory> Packers::concatBuffers(const tBufVec& bufVec) {
  std::shared_ptr<Memory> concatBuf = Memory::makeNew(mSrcBytes);
  uint32_t concatBufOffset = 0;
  for (tBufVec::const_iterator it = bufVec.begin(); it != bufVec.end(); ++it) {
    const uint8_t* buf = it->first;
    uint32_t len = it->second;
    memcpy (concatBuf->buf() + concatBufOffset, buf, len);
    concatBufOffset += len;
  }
  return concatBuf;
} 

std::shared_ptr<Memory> Packers::convert(std::shared_ptr<Memory> srcBuf) {
  // only 420P supported currently
  uint32_t lumaBytes = mSrcWidth * mSrcHeight;
  std::shared_ptr<Memory> convertBuf = Memory::makeNew(lumaBytes * 3 / 2);
  if (0 == mSrcFmtCode.compare("4175")) {
    convertPGroupto420P (srcBuf->buf(), convertBuf->buf());
  }
  else if (0 == mSrcFmtCode.compare("v210")) {
    convertV210to420P (srcBuf->buf(), convertBuf->buf());
  }
  return convertBuf;  
}

// private
void Packers::convertPGroupto420P (const uint8_t *const srcBuf, uint8_t *const dstBuf) {
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
    if (evenLine) {
      dstULine += dstChromaPitchBytes;
      dstVLine += dstChromaPitchBytes;
    } 
  }
}

void Packers::convertV210to420P (const uint8_t *const srcBuf, uint8_t *const dstBuf) {
// V210: https://developer.apple.com/library/mac/technotes/tn2162/_index.html#//apple_ref/doc/uid/DTS40013070-CH1-TNTAG8-V210__4_2_2_COMPRESSION_TYPE
// 420P: https://en.wikipedia.org/wiki/YUV
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
    for (uint32_t x=0; x<mSrcWidth; x+=6) {
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

    srcLine += srcPitchBytes;
    dstYLine += dstLumaPitchBytes; 
    if (evenLine) {
      dstULine += dstChromaPitchBytes;
      dstVLine += dstChromaPitchBytes;
    } 
  }
}

} // namespace streampunk
