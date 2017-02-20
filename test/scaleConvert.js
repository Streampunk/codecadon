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

var test = require('tape');
var codecadon = require('../../codecadon');

function make4175Buf(width, height) {
  var pitchBytes = width * 5 / 2;
  var buf = Buffer.alloc(pitchBytes * height);  
  var yOff = 0;
  for (var y=0; y<height; ++y) {
    var xOff = 0;
    for (var x=0; x<width; ++x) {
      // uyvy, big-endian 10 bits each in 5 bytes
      buf[yOff + xOff++] = 0x80;       
      buf[yOff + xOff++] = 0x04;       
      buf[yOff + xOff++] = 0x08;       
      buf[yOff + xOff++] = 0x00;       
      buf[yOff + xOff++] = 0x40;       
    }
    yOff += pitchBytes;
  }   
  return buf;
}

function make420PBuf(width, height) {
  var lumaPitchBytes = width;
  var chromaPitchBytes = lumaPitchBytes / 2;
  var buf = Buffer.alloc(lumaPitchBytes * height * 3 / 2);
  var lOff = 0;
  var uOff = lumaPitchBytes * height;
  var vOff = uOff + chromaPitchBytes * height / 2;

  for (var y=0; y<height; ++y) {
    var xlOff = 0;
    var xcOff = 0;
    var evenLine = (y & 1) === 0;
    for (var x=0; x<width; x+=2) {
      buf[lOff + xlOff + 0] = 0x10;
      buf[lOff + xlOff + 1] = 0x10;
      buf[uOff + xcOff] = 0x80;    
      buf[vOff + xcOff] = 0x80;
      xlOff += 2;
      xcOff += 1;
    }
    lOff += lumaPitchBytes;
    if (!evenLine) {
      uOff += chromaPitchBytes;
      vOff += chromaPitchBytes;
    }
  }
  return buf;
}

function makeYUV422P10Buf(width, height) {
  var lumaPitchBytes = width * 2;
  var chromaPitchBytes = lumaPitchBytes / 2;
  var buf = Buffer.alloc(lumaPitchBytes * height * 2);  
  var lOff = 0;
  var uOff = lumaPitchBytes * height;
  var vOff = uOff + chromaPitchBytes * height;

  for (var y=0; y<height; ++y) {
    var xlOff = 0;
    var xcOff = 0;
    for (var x=0; x<width; x+=2) {
      buf.writeUInt16LE(0x0040, lOff + xlOff);
      buf.writeUInt16LE(0x0040, lOff + xlOff + 2);
      xlOff += 4;
    
      buf.writeUInt16LE(0x0200, uOff + xcOff);
      buf.writeUInt16LE(0x0200, vOff + xcOff);
      xcOff += 2;
    }
    lOff += lumaPitchBytes;
    uOff += chromaPitchBytes;
    vOff += chromaPitchBytes;
  }
  return buf;
}

function makeTags(width, height, packing, interlace) {
  this.tags = [];
  this.tags["format"] = [ "video" ];
  this.tags["width"] = [ `${width}` ];
  this.tags["height"] = [ `${height}` ];
  this.tags["packing"] = [ packing ];
  var depth = 8;
  if ("420P" !== packing)
    depth = 10;
  this.tags["depth"] = [ `${depth}` ];
  this.tags["interlace"] = [ `${interlace}` ];
  return tags;
}

function scaleConvertTest(description, onErr, fn) {
  test(description, function (t) {
    var scaleConverter = new codecadon.ScaleConverter(function() {
      t.end();
    });
    scaleConverter.on('error', function(err) {
      onErr(t, err);
    });

    fn(t, scaleConverter, function() {
      scaleConverter.quit(function(err, result) {});
    });
  });
};

function badDims() {
  var srcTags = makeTags(1280, 720, 'pgroup', 0);
  var dstTags = makeTags(21, 0, '420P', 0);
  scaleConverter.setInfo(srcTags, dstTags);
}

function badFmt() {
  var srcTags = makeTags(1280, 720, 'pgroup', 0);
  var dstTags = makeTags(1920, 1080, 'pgroup', 0);
  scaleConverter.setInfo(srcTags, dstTags);
}

scaleConvertTest('Handling bad image dimensions', 
  function (t, err) {
    t.ok(err, 'emits error');
  }, 
  function (t, scaleConverter, done) {
    t.throws(badDims);
    done();
  });

scaleConvertTest('Handling bad image format',
  function (t, err) {
    t.ok(err, 'emits error');
  }, 
  function (t, scaleConverter, done) {
    t.throws(badFmt);
    done();
  });

scaleConvertTest('Starting up a scaleConverter', 
  function (t, err) {
    t.notOk(err, 'no error expected');
  }, 
  function (t, scaleConverter, done) {
    var dstWidth = 1920;
    var dstHeight = 1080;
    var srcTags = makeTags(1280, 720, 'pgroup', 0);
    var dstTags = makeTags(dstWidth, dstHeight, '420P', 0);
  
    var dstBufLen = scaleConverter.setInfo(srcTags, dstTags);
    var numBytesExpected = dstWidth * dstHeight * 3/2;
    t.equal(dstBufLen, numBytesExpected, 'buffer size calculation matches the expected value');
    done();
  });

scaleConvertTest('Performing scaling pgroup to YUV422P10',
  function (t, err) {
    t.notOk(err, 'no error expected');
  }, 
  function (t, scaleConverter, done) {
    var srcWidth = 1920;
    var srcHeight = 1080;
    var srcFormat = 'pgroup';
    var dstWidth = 1280;
    var dstHeight = 720;
    var dstFormat = 'YUV422P10';
    var srcTags = makeTags(srcWidth, srcHeight, srcFormat, 1);
    var dstTags = makeTags(dstWidth, dstHeight, dstFormat, 1);
    var dstBufLen = scaleConverter.setInfo(srcTags, dstTags);
    var bufArray = new Array(1);
    var srcBuf = make4175Buf(srcWidth, srcHeight);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    var numQueued = scaleConverter.scaleConvert(bufArray, dstBuf, function(err, result) {
      t.notOk(err, 'no error expected');
      var testDstBuf = makeYUV422P10Buf(dstWidth, dstHeight);
      t.deepEquals(result, testDstBuf, 'matches the expected scaling result')   
      done();
    });
  });

scaleConvertTest('Handling undefined source',
  function (t, err) {
    t.notOk(err, 'no error expected');
  }, 
  function (t, scaleConverter, done) {
    var bufArray;
    var srcWidth = 1920;
    var srcHeight = 1080;
    var srcFormat = 'pgroup';
    var dstWidth = 1920;
    var dstHeight = 1080;
    var dstFormat = '420P';
    var srcTags = makeTags(srcWidth, srcHeight, srcFormat, 0);
    var dstTags = makeTags(dstWidth, dstHeight, dstFormat, 0);
    var dstBufLen = scaleConverter.setInfo(srcTags, dstTags);
    var dstBuf = Buffer.alloc(dstBufLen);
    scaleConverter.scaleConvert(bufArray, dstBuf, function(err, result) {
      t.ok(err, 'should return error');
      done();
    });
  });

scaleConvertTest('Handling undefined destination', 
  function (t, err) {
    t.notOk(err, 'no error expected');
  }, 
  function (t, scaleConverter, done) {
    var srcWidth = 1920;
    var srcHeight = 1080;
    var srcFormat = 'pgroup';
    var dstWidth = 1920;
    var dstHeight = 1080;
    var dstFormat = '420P';
    var srcTags = makeTags(srcWidth, srcHeight, srcFormat, 0);
    var dstTags = makeTags(dstWidth, dstHeight, dstFormat, 0);
    var dstBufLen = scaleConverter.setInfo(srcTags, dstTags);
    var bufArray = new Array(1);
    var srcBuf = make4175Buf(srcWidth, srcHeight);
    bufArray[0] = srcBuf;
    var dstBuf;
    scaleConverter.scaleConvert(bufArray, dstBuf, function(err, result) {
      t.ok(err, 'should return error');
      done();
    });
  });

scaleConvertTest('Handling insufficient destination bytes', 
  function (t, err) {
    t.notOk(err, 'no error expected');
  }, 
  function (t, scaleConverter, done) {
    var srcWidth = 1920;
    var srcHeight = 1080;
    var srcFormat = 'pgroup';
    var dstWidth = 1920;
    var dstHeight = 1080;
    var dstFormat = '420P';
    var srcTags = makeTags(srcWidth, srcHeight, srcFormat, 0);
    var dstTags = makeTags(dstWidth, dstHeight, dstFormat, 0);
    var dstBufLen = scaleConverter.setInfo(srcTags, dstTags);
    var bufArray = new Array(1);
    var srcBuf = make4175Buf(srcWidth, srcHeight);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen - 128);
    scaleConverter.scaleConvert(bufArray, dstBuf, function(err, result) {
      t.ok(err, 'should return error');
      done();
    });
  });
