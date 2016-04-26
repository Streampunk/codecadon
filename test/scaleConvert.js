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

function makeTags(width, height, packing, interlace) {
  this.tags = [];
  this.tags["width"] = [ `${width}` ];
  this.tags["height"] = [ `${height}` ];
  this.tags["packing"] = [ packing ];
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

scaleConvertTest('Handling bad image dimensions', 
  function (t, err) {
    t.ok(err, 'emits error');
  }, 
  function (t, scaleConverter, done) {
    var srcTags = makeTags(1280, 720, 'pgroup', 0);
    var dstTags = makeTags(21, 0, '420P', 0);
    t.throws(scaleConverter.setInfo(srcTags, dstTags));
    done();
  });

scaleConvertTest('Handling bad image format',
  function (t, err) {
    t.ok(err, 'emits error');
  }, 
  function (t, scaleConverter, done) {
    var srcTags = makeTags(1280, 720, 'pgroup', 0);
    var dstTags = makeTags(1920, 1080, 'pgroup', 0);
    t.throws(scaleConverter.setInfo(srcTags, dstTags));
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

scaleConvertTest('Performing packing',
  function (t, err) {
    t.notOk(err, 'no error expected');
  }, 
  function (t, scaleConverter, done) {
    var width = 1920;
    var height = 1080;
    var srcTags = makeTags(width, height, 'pgroup', 0);
    var dstTags = makeTags(width, height, '420P', 0);
    var dstBufLen = scaleConverter.setInfo(srcTags, dstTags);

    var bufArray = make4175BufArray(width, height);
    var dstBuf = new Buffer(dstBufLen);
    var numQueued = scaleConverter.scaleConvert(bufArray, dstBuf, function(err, result) {
      t.notOk(err, 'no error expected');
      var testDstBuf = make420PBuf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result')   
      done();
    });
  });

scaleConvertTest('Performing scaling',
  function (t, err) {
    t.notOk(err, 'no error expected');
  }, 
  function (t, scaleConverter, done) {
    var srcWidth = 1920;
    var srcHeight = 1080;
    var srcFormat = 'pgroup';
    var dstWidth = 1280;
    var dstHeight = 720;
    var dstFormat = '420P';
    var srcTags = makeTags(srcWidth, srcHeight, srcFormat, 1);
    var dstTags = makeTags(dstWidth, dstHeight, dstFormat, 1);
    var dstBufLen = scaleConverter.setInfo(srcTags, dstTags);
    var bufArray = make4175BufArray(srcWidth, srcHeight);
    var dstBuf = new Buffer(dstBufLen);
    var numQueued = scaleConverter.scaleConvert(bufArray, dstBuf, function(err, result) {
      t.notOk(err, 'no error expected');
      var testDstBuf = make420PBuf(dstWidth, dstHeight);
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
    var dstBuf = new Buffer(dstBufLen);
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
    var bufArray = make4175BufArray(srcWidth, srcHeight);
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
    var bufArray = make4175BufArray(srcWidth, srcHeight);
    var dstBuf = new Buffer(dstBufLen - 128);
    scaleConverter.scaleConvert(bufArray, dstBuf, function(err, result) {
      t.ok(err, 'should return error');
      done();
    });
  });
