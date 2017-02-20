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

function makeBufArray(bytesPerBuf, numBuffers) {
  var a = new Array(numBuffers);
  var c = 0;
  for (var b=0; b<numBuffers; ++b) {
    var buf = Buffer.alloc(bytesPerBuf);
    for (var i=0; i<bytesPerBuf; ++i) {
      buf[i] = (c++)&0xff;
    }
    a[b] = buf; 
  }
  return a;
}

function makeTags(width, height) {
  this.tags = [];
  this.tags["format"] = [ "video" ];
  this.tags["width"] = [ `${width}` ];
  this.tags["height"] = [ `${height}` ];
  this.tags["interlace"] = [ "false" ];
  return tags;
}

function concatTest(description, fn) {
  test(description, function (t) {
    var concater = new codecadon.Concater(function() {
      t.end();
    });
    concater.on('error', function(err) { 
      t.fail(err); 
    });

    fn(t, concater, function() {
      concater.quit(function(err, result) {});
    });
  });
};

concatTest('Performing concatenation', function (t, concater, done) {
  var width = 1920;
  var height = 1080;
  var numBuffers = 128;
  var tags = makeTags(width, height);
  var numBytes = concater.setInfo(tags);
  var bufArray = makeBufArray(numBytes / numBuffers, numBuffers);
  var dstBuf = Buffer.alloc(numBytes);
  concater.concat(bufArray, dstBuf, function(err, result) {
    t.notOk(err, 'no error expected');
    var testDstBuf = makeBufArray(numBytes, 1)[0];
    t.deepEquals(result, testDstBuf, 'matches the expected concatenation result')   
    done();
  });
});

concatTest('Handling an undefined source buffer array', function (t, concater, done) {
  var width = 1920;
  var height = 1080;
  var tags = makeTags(width, height);
  var numBytes = concater.setInfo(tags);
  var bufArray;
  var dstBuf = Buffer.alloc(numBytes);
  concater.concat(bufArray, dstBuf, function(err, result) {
    t.ok(err, 'should return error');
    done();
  });
});

concatTest('Handling an undefined destination buffer', function (t, concater, done) {
  var width = 1920;
  var height = 1080;
  var numBuffers = 128;
  var tags = makeTags(width, height);
  var numBytes = concater.setInfo(tags);
  var bufArray = makeBufArray(numBytes, numBuffers);
  var dstBuf;
  concater.concat(bufArray, dstBuf, function(err, result) {
    t.ok(err, 'should return error');
    done();
  });
});

concatTest('Handling source buffer array bytes being greater than destination bytes', function (t, concater, done) {
  var width = 1920;
  var height = 1080;
  var numBuffers = 128;
  var tags = makeTags(width, height);
  var numBytes = concater.setInfo(tags);
  var bufArray = makeBufArray(numBytes, numBuffers + 10);
  var dstBuf = Buffer.alloc(numBytes);
  concater.concat(bufArray, dstBuf, function(err, result) {
    t.ok(err, 'should return error');
    done();
  });
});

