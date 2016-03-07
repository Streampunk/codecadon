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

function concatTest(description, format, width, height, fn) {
  test(description, function (t) {
    var concater = new codecadon.Concater(format, width, height);
    concater.on('exit', function() {
      concater.finish();
      t.end();
    });
    concater.on('error', function(err) { 
      t.fail(err); 
    });

    fn(t, concater, format, width, height, function() {
      concater.quit(function() {});
    });
  });
};

function createBad1(format, width, height) {
  new codecadon.Concater('4175', 21, 0);
};
function createBad2(format, width, height) {
  new codecadon.Concater('4176', 1920, 1080);
};

test('Handle bad image dimensions', function (t) {
  t.throws(createBad1);
  t.end();
});

test('Handle bad image format', function (t) {
  t.throws(createBad2);
  t.end();
});

concatTest('Startup a concatenator', '4175', 1920, 1080, function (t, concater, format, width, height, done) {
  var dstBufLen = concater.start();
  var numBytesExpected = width * height * 5/2;
  t.equal(dstBufLen, numBytesExpected, 'testing buffer size calculation');
  done();
});

concatTest('Perform concatenation', '4175', 1920, 1080, function (t, concater, format, width, height, done) {
  var numBytes = width * height * 5/2;
  var bufArray = makeBufArray(numBytes / width, width);
  var dstBufLen = concater.start();
  var dstBuf = new Buffer(dstBufLen);
  var numQueued = concater.concat(bufArray, dstBuf, function(err, result) {
    t.notOk(err, 'no error');
    var testDstBuf = makeBufArray(numBytes, 1)[0];
    t.deepEquals(result, testDstBuf, 'testing concatenation result')   
    done();
  });
});

concatTest('Handle undefined source', '4175', 1920, 1080, function (t, concater, format, width, height, done) {
  var bufArray;
  var dstBufLen = concater.start();
  var dstBuf = new Buffer(dstBufLen);
  concater.concat(bufArray, dstBuf, function(err, result) {
    t.ok(err, 'should return error');
    done();
  });
});

concatTest('Handle undefined destination', '4175', 1920, 1080, function (t, concater, format, width, height, done) {
  var numBytes = width * height * 5/2;
  var bufArray = makeBufArray(numBytes / width, width);
  concater.start();
  var dstBuf;
  concater.concat(bufArray, dstBuf, function(err, result) {
    t.ok(err, 'should return error');
    done();
  });
});

concatTest('Handle source bytes greater than destination bytes', '4175', 1920, 1080, function (t, concater, format, width, height, done) {
  var numBytes = width * height * 5/2;
  var bufArray = makeBufArray(numBytes / width, width + 10);
  var dstBufLen = concater.start();
  var dstBuf = new Buffer(dstBufLen);
  concater.concat(bufArray, dstBuf, function(err, result) {
    t.notOk(err, 'no error');
    var testDstBuf = makeBufArray(numBytes, 1)[0];
    t.deepEquals(result, testDstBuf, 'testing concatenation result')   
    done();
  });
});

