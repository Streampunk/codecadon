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

function makeBufArray(bytesPerBuf, numBuffers) {
  var a = new Array(numBuffers);
  var c = 0;
  for (var b=0; b<numBuffers; ++b) {
    var buf = new Buffer(bytesPerBuf);
    for (var i=0; i<bytesPerBuf; ++i) {
      buf[i] = (c++)&0xff;
    }
    a[b] = buf; 
  }
  return a;
}

function concatTest(description, fn) {
  test(description, function (t) {
    var concater = new codecadon.Concater;
    concater.on('exit', function() {
      concater.finish();
      t.end();
    });
    concater.on('error', function(err) { 
      t.fail(err); 
    });

    fn(t, concater, function() {
      concater.quit(function() {});
    });
  });
};

concatTest('Perform concatenation', function (t, concater, done) {
  var width = 1920;
  var height = 1080;
  var numBytes = width * height * 5/2;
  var bufArray = makeBufArray(numBytes / width, width);
  concater.start();
  var dstBuf = new Buffer(numBytes);
  concater.concat(bufArray, dstBuf, function(err, result) {
    t.notOk(err, 'no error');
    var testDstBuf = makeBufArray(numBytes, 1)[0];
    t.deepEquals(result, testDstBuf, 'testing concatenation result')   
    done();
  });
});

concatTest('Handle undefined source', function (t, concater, done) {
  var width = 1920;
  var height = 1080;
  var numBytes = width * height * 5/2;
  var bufArray;
  concater.start();
  var dstBuf = new Buffer(numBytes);
  concater.concat(bufArray, dstBuf, function(err, result) {
    t.ok(err, 'should return error');
    done();
  });
});

concatTest('Handle undefined destination', function (t, concater, done) {
  var width = 1920;
  var height = 1080;
  var numBytes = width * height * 5/2;
  var bufArray = makeBufArray(numBytes / width, width);
  concater.start();
  var dstBuf;
  concater.concat(bufArray, dstBuf, function(err, result) {
    t.ok(err, 'should return error');
    done();
  });
});

concatTest('Handle source bytes greater than destination bytes', function (t, concater, done) {
  var width = 1920;
  var height = 1080;
  var numBytes = width * height * 5/2;
  var bufArray = makeBufArray(numBytes / width, width + 10);
  concater.start();
  var dstBuf = new Buffer(numBytes);
  concater.concat(bufArray, dstBuf, function(err, result) {
    t.ok(err, 'should return error');
    done();
  });
});

