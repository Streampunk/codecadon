/* Copyright 2015 Christine S. MacNeill

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by appli cable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

var test = require('tape');
var codecadon = require('../../codecadon');

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


function encodeTest(description, format, width, height, fn) {
  test(description, function (t) {
    var encoder = new codecadon.Encoder(format, width, height);
    encoder.on('exit', function() {
      encoder.finish();
      t.end();
    });
    encoder.on('error', function(err) {
      t.fail(err); 
    });

    fn(t, encoder, format, width, height, function() {
      encoder.quit(function() {});
    });
  });
};

function createBad1() {
  new codecadon.Encoder('h264', 21, 0);
};
function createBad2() {
  new codecadon.Encoder('420P', 1920, 1080);
};

test('Handle bad image dimensions', function (t) {
  t.throws(createBad1);
  t.end();
});

test('Handle bad image format', function (t) {
  t.throws(createBad2);
  t.end();
});

encodeTest('Startup an encoder', 'h264', 1920, 1080, function (t, encoder, dstFormat, dstWidth, dstHeight, done) {
  var dstBufLen = encoder.start();
  var numBytesExpected = dstWidth * dstHeight;
  t.equal(dstBufLen, numBytesExpected, 'testing buffer size calculation');
  done();
});

encodeTest('Perform encoding', 'h264', 1920, 1080, function (t, encoder, dstFormat, dstWidth, dstHeight, done) {
  var srcWidth = 1920;
  var srcHeight = 1080;
  var srcFormat = '420P';
  var bufArray = new Array(1); 
  bufArray[0] = make420PBuf(srcWidth, srcHeight);
  var dstBufLen = encoder.start();
  var dstBuf = new Buffer(dstBufLen);
  var numQueued = encoder.encode(bufArray, srcWidth, srcHeight, srcFormat, dstBuf, function(err, result) {
    t.notOk(err, 'no error');
    // todo: check for valid bitstream...
    done();
  });
});

encodeTest('Handle undefined source', 'h264', 1920, 1080, function (t, encoder, dstFormat, dstWidth, dstHeight, done) {
  var bufArray;
  var dstBufLen = encoder.start();
  var dstBuf = new Buffer(dstBufLen);
  encoder.encode(bufArray, 1920, 1080, '420P', dstBuf, function(err, result) {
    t.ok(err, 'should return error');
    done();
  });
});

encodeTest('Handle undefined destination', 'h264', 1920, 1080, function (t, encoder, dstFormat, dstWidth, dstHeight, done) {
  var srcWidth = 1920;
  var srcHeight = 1080;
  var srcFormat = '420P';
  var bufArray = new Array(1); 
  bufArray[0] = make420PBuf(srcWidth, srcHeight);
  encoder.start();
  var dstBuf;
  encoder.encode(bufArray, srcWidth, srcHeight, srcFormat, dstBuf, function(err, result) {
    t.ok(err, 'should return error');
    done();
  });
});

