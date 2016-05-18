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

function makeTags(width, height, packing, interlace) {
  this.tags = [];
  this.tags["width"] = [ `${width}` ];
  this.tags["height"] = [ `${height}` ];
  this.tags["packing"] = [ packing ];
  this.tags["interlace"] = [ `${interlace}` ];
  return tags;
}

function encodeTest(description, onErr, fn) {
  test(description, function (t) {
    var encoder = new codecadon.Encoder(function() {
      t.end();
    });
    encoder.on('error', function(err) {
      onErr(t, err);
    });

    fn(t, encoder, function() {
      encoder.quit(function() {});
    });
  });
};

encodeTest('Handling bad image dimensions',
  function (t, err) {
    t.ok(err, 'emits error');
  }, 
  function (t, encoder, done) {
    var srcTags = makeTags(1280, 720, 'pgroup', 0);
    var dstTags = makeTags(21, 0, 'h264', 0);
    t.throws(encoder.setInfo(srcTags, dstTags));
    done();
  });

encodeTest('Handling bad image format',
  function (t, err) {
    t.ok(err, 'emits error');
  }, 
  function (t, encoder, done) {
    var srcTags = makeTags(1920, 1080, 'pgroup', 0);
    var dstTags = makeTags(1920, 1080, '420P', 0);
    t.throws(encoder.setInfo(srcTags, dstTags));
    done();
  });

encodeTest('Starting up an encoder', 
  function (t, err) {
    t.notOk(err, 'no error expected');
  }, 
  function (t, encoder, done) {
    var dstWidth = 1920; 
    var dstHeight = 1080; 
    var srcTags = makeTags(1920, 1080, '420P', 0);
    var dstTags = makeTags(dstWidth, dstHeight, 'h264', 0);
    var dstBufLen = encoder.setInfo(srcTags, dstTags);
    var numBytesExpected = dstWidth * dstHeight;
    t.equal(dstBufLen, numBytesExpected, 'buffer size calculation matches the expected value');
    done();
  });

encodeTest('Performing encoding',
  function (t, err) {
    t.notOk(err, 'no error expected');
  }, 
  function (t, encoder, done) {
    var srcWidth = 1920;
    var srcHeight = 1080;
    var srcFormat = '420P';
    var dstWidth = 1920; 
    var dstHeight = 1080; 
    var dstFormat = 'h264';
    var srcTags = makeTags(srcWidth, srcHeight, srcFormat, 0);
    var dstTags = makeTags(dstWidth, dstHeight, dstFormat, 0);
    var bufArray = new Array(1); 
    bufArray[0] = make420PBuf(srcWidth, srcHeight);
    var dstBufLen = encoder.setInfo(srcTags, dstTags);
    var dstBuf = new Buffer(dstBufLen);
    var numQueued = encoder.encode(bufArray, dstBuf, function(err, result) {
      t.notOk(err, 'no error expected');
      // todo: check for valid bitstream...
      done();
    });
  });

encodeTest('Handling an undefined source buffer array', 
  function (t, err) {
    t.notOk(err, 'no error expected');
  }, 
  function (t, encoder, done) {
    var srcWidth = 1920;
    var srcHeight = 1080;
    var srcFormat = '420P';
    var dstWidth = 1920; 
    var dstHeight = 1080; 
    var dstFormat = 'h264';
    var srcTags = makeTags(srcWidth, srcHeight, srcFormat, 0);
    var dstTags = makeTags(dstWidth, dstHeight, dstFormat, 0);
    var dstBufLen = encoder.setInfo(srcTags, dstTags);
    var bufArray;
    var dstBuf = new Buffer(dstBufLen);
    encoder.encode(bufArray, dstBuf, function(err, result) {
      t.ok(err, 'should return error');
      done();
    });
  });

encodeTest('Handling an undefined destination buffer',
  function (t, err) {
    t.notOk(err, 'no error expected');
  }, 
  function (t, encoder, done) {
    var srcWidth = 1920;
    var srcHeight = 1080;
    var srcFormat = '420P';
    var dstWidth = 1920; 
    var dstHeight = 1080; 
    var dstFormat = 'h264';
    var srcTags = makeTags(srcWidth, srcHeight, srcFormat, 0);
    var dstTags = makeTags(dstWidth, dstHeight, dstFormat, 0);
    var dstBufLen = encoder.setInfo(srcTags, dstTags);
    var bufArray = new Array(1); 
    bufArray[0] = make420PBuf(srcWidth, srcHeight);
    var dstBuf;
    encoder.encode(bufArray, dstBuf, function(err, result) {
      t.ok(err, 'should return error');
      done();
    });
  });
