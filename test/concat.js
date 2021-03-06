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

var tap = require('tap');
var codecadon = require('../../codecadon');
const logLevel = 2;

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
  let tags = {};
  tags.format = 'video';
  tags.width = width;
  tags.height = height;
  tags.interlace = false;
  return tags;
}

function concatTest(description, numTests, onErr, fn) {
  tap.test(description, t => {
    t.plan(numTests + 1);
    var concater = new codecadon.Concater(() => {});
    concater.on('error', err => { 
      onErr(err);
    });

    fn(t, concater, () => {
      concater.quit(() => {
        t.pass(`${description} exited`);
        t.end();
      });
    });
  });
}

tap.plan(4, 'Concatenator addon tests');

concatTest('Performing concatenation', 2,
  (t, err) => t.notOk(err, 'no error expected'),
  (t, concater, done) => {
    var width = 1920;
    var height = 1080;
    var numBuffers = 128;
    var tags = makeTags(width, height);
    var numBytes = concater.setInfo(tags, logLevel);
    var bufArray = makeBufArray(numBytes / numBuffers, numBuffers);
    var dstBuf = Buffer.alloc(numBytes);
    concater.concat(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = makeBufArray(numBytes, 1)[0];
      t.deepEquals(result, testDstBuf, 'matches the expected concatenation result');
      done();
    });
  });

concatTest('Handling an undefined source buffer array', 1,
  (t, err) => t.notOk(err, 'no error expected'),
  (t, concater, done) => {
    var width = 1920;
    var height = 1080;
    var tags = makeTags(width, height);
    var numBytes = concater.setInfo(tags, logLevel);
    var bufArray;
    var dstBuf = Buffer.alloc(numBytes);
    concater.concat(bufArray, dstBuf, (err/*, result*/) => {
      t.ok(err, 'should return error');
      done();
    });
  });

concatTest('Handling an undefined destination buffer', 1,
  (t, err) => t.notOk(err, 'no error expected'),
  (t, concater, done) => {
    var width = 256;
    var height = 64;
    var numBuffers = 16;
    var tags = makeTags(width, height);
    var numBytes = concater.setInfo(tags, logLevel);
    var bufArray = makeBufArray(numBytes, numBuffers);
    var dstBuf;
    concater.concat(bufArray, dstBuf, (err/*, result*/) => {
      t.ok(err, 'should return error');
      done();
    });
  });

concatTest('Handling source buffer array bytes being greater than destination bytes', 1,
  (t, err) => t.notOk(err, 'no error expected'),
  (t, concater, done) => {
    var width = 256;
    var height = 64;
    var numBuffers = 16;
    var tags = makeTags(width, height);
    var numBytes = concater.setInfo(tags, logLevel);
    var bufArray = makeBufArray(numBytes, numBuffers + 2);
    var dstBuf = Buffer.alloc(numBytes);
    concater.concat(bufArray, dstBuf, (err/*, result*/) => {
      t.ok(err, 'should return error');
      done();
    });
  });

