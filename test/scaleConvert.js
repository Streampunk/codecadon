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

var test = require('tape');
var codecadon = require('../../codecadon');

function make4175BufArray(width, height) {
  var a = new Array(1);
  var pitchBytes = width * 5 / 2;
  var buf = new Buffer(pitchBytes * height);  
  var yOff = 0;
  for (var y=0; y<height; ++y) {
    var xOff = 0;
    for (var x=0; x<width; ++x) {
      // uyvy, 10 bits each in 5 bytes
      buf[yOff + xOff++] = 0x80;       
      buf[yOff + xOff++] = 0x04;       
      buf[yOff + xOff++] = 0x08;       
      buf[yOff + xOff++] = 0x00;       
      buf[yOff + xOff++] = 0x40;       
    }
    yOff += pitchBytes;
  }   
  a[0] = buf;
  return a;
}

function make420PBuf(width, height) {
  var lumaPitchBytes = width;
  var chromaPitchBytes = lumaPitchBytes / 2;
  var buf = new Buffer(lumaPitchBytes * height * 3 / 2);  
  var lOff = 0;
  var uOff = lumaPitchBytes * height;
  var vOff = uOff + chromaPitchBytes * height / 2;

  for (var y=0; y<height; ++y) {
    var xlOff = 0;
    var xcOff = 0;
    var evenLine = (y & 1) == 0;
    for (var x=0; x<width; x+=2) {
      buf[lOff + xlOff++] = 0x10;
      buf[lOff + xlOff++] = 0x10;
    
      if (evenLine) {
        buf[uOff + xcOff] = 0x80;    
        buf[vOff + xcOff] = 0x80;
        xcOff++;
      }    
    }
    lOff += lumaPitchBytes;
    if (!evenLine) {
      uOff += chromaPitchBytes;
      vOff += chromaPitchBytes;
    }
  }
  return buf;
}


function scaleConvertTest(description, format, width, height, fn) {
  test(description, function (t) {
    var scaleConverter = new codecadon.ScaleConverter(format, width, height);
    scaleConverter.on('exit', function() {
      scaleConverter.finish();
      t.end();
    });
    scaleConverter.on('error', function(err) {
      t.fail(err); 
    });

    fn(t, scaleConverter, format, width, height, function() {
      scaleConverter.quit(function() {});
    });
  });
};

function createBad1() {
  new codecadon.ScaleConverter('420P', 21, 0);
};
function createBad2() {
  new codecadon.ScaleConverter('4175', 1920, 1080);
};

test('Handle bad image dimensions', function (t) {
  t.throws(createBad1);
  t.end();
});

test('Handle bad image format', function (t) {
  t.throws(createBad2);
  t.end();
});

scaleConvertTest('Startup a scaleConverter', '420P', 1920, 1080, function (t, scaleConverter, dstFormat, dstWidth, dstHeight, done) {
  var dstBufLen = scaleConverter.start();
  var numBytesExpected = dstWidth * dstHeight * 3/2;
  t.equal(dstBufLen, numBytesExpected, 'testing buffer size calculation');
  done();
});

scaleConvertTest('Perform packing', '420P', 1920, 1080, function (t, scaleConverter, dstFormat, dstWidth, dstHeight, done) {
  var srcWidth = 1920;
  var srcHeight = 1080;
  var srcFormat = '4175';
  var bufArray = make4175BufArray(srcWidth, srcHeight);
  var dstBufLen = scaleConverter.start();
  var dstBuf = new Buffer(dstBufLen);
  var numQueued = scaleConverter.scaleConvert(bufArray, srcWidth, srcHeight, srcFormat, dstBuf, function(err, result) {
    t.notOk(err, 'no error');
    var testDstBuf = make420PBuf(dstWidth, dstHeight);
    t.deepEquals(result, testDstBuf, 'testing packing result')   
    done();
  });
});

scaleConvertTest('Perform scaling', '420P', 1280, 720, function (t, scaleConverter, dstFormat, dstWidth, dstHeight, done) {
  var srcWidth = 1920;
  var srcHeight = 1080;
  var srcFormat = '4175';
  var bufArray = make4175BufArray(srcWidth, srcHeight);
  var dstBufLen = scaleConverter.start();
  var dstBuf = new Buffer(dstBufLen);
  var numQueued = scaleConverter.scaleConvert(bufArray, srcWidth, srcHeight, srcFormat, dstBuf, function(err, result) {
    t.notOk(err, 'no error');
    var testDstBuf = make420PBuf(dstWidth, dstHeight);
    t.deepEquals(result, testDstBuf, 'testing scaling result')   
    done();
  });
});

scaleConvertTest('Handle undefined source', '420P', 1920, 1080, function (t, scaleConverter, dstFormat, dstWidth, dstHeight, done) {
  var bufArray;
  var dstBufLen = scaleConverter.start();
  var dstBuf = new Buffer(dstBufLen);
  scaleConverter.scaleConvert(bufArray, 1920, 1080, '4175', dstBuf, function(err, result) {
    t.ok(err, 'should return error');
    done();
  });
});

scaleConvertTest('Handle undefined destination', '420P', 1920, 1080, function (t, scaleConverter, dstFormat, dstWidth, dstHeight, done) {
  var srcWidth = 1920;
  var srcHeight = 1080;
  var srcFormat = '4175';
  var bufArray = make4175BufArray(srcWidth, srcHeight);
  scaleConverter.start();
  var dstBuf;
  scaleConverter.scaleConvert(bufArray, srcWidth, srcHeight, srcFormat, dstBuf, function(err, result) {
    t.ok(err, 'should return error');
    done();
  });
});

scaleConvertTest('Handle insufficient destination bytes', '420P', 1920, 1080, function (t, scaleConverter, dstFormat, dstWidth, dstHeight, done) {
  var srcWidth = 1920;
  var srcHeight = 1080;
  var srcFormat = '4175';
  var bufArray = make4175BufArray(srcWidth, srcHeight);
  var dstBufLen = scaleConverter.start();
  var dstBuf = new Buffer(dstBufLen - 128);
  scaleConverter.scaleConvert(bufArray, srcWidth, srcHeight, srcFormat, dstBuf, function(err, result) {
    t.ok(err, 'should return error');
    done();
  });
});

