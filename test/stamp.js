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

function make420PBuf(width, height, wipeVal) {
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
      buf[lOff + xlOff + 0] = wipeVal.y;
      buf[lOff + xlOff + 1] = wipeVal.y;
      buf[uOff + xcOff] = wipeVal.cb;    
      buf[vOff + xcOff] = wipeVal.cr;
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

function makeYUV422P10Buf(width, height, wipeVal) {
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
      buf.writeUInt16LE(wipeVal.y, lOff + xlOff);
      buf.writeUInt16LE(wipeVal.y, lOff + xlOff + 2);
      xlOff += 4;
    
      buf.writeUInt16LE(wipeVal.cb, uOff + xcOff);
      buf.writeUInt16LE(wipeVal.cr, vOff + xcOff);
      xcOff += 2;
    }
    lOff += lumaPitchBytes;
    uOff += chromaPitchBytes;
    vOff += chromaPitchBytes;
  }
  return buf;
}

function makeTags(width, height, packing, interlace) {
  let tags = {};
  tags.format = 'video';
  tags.width = width;
  tags.height = height;
  tags.packing = packing;
  tags.interlace = interlace;
  return tags;
}

function stampTest(description, numTests, onErr, fn) {
  test(description, (t) => {
    t.plan(numTests + 1);
    var stamper = new codecadon.Stamper(() => t.pass(`${description} exited`));
    stamper.on('error', err => {
      onErr(t, err);
    });

    fn(t, stamper, () => {
      stamper.quit(() => {});
    });
  });
}

stampTest('Starting up a stamper', 1,
  (t, err) => t.notOk(err, 'no error expected'), 
  (t, stamper, done) => {
    var dstWidth = 1920;
    var dstHeight = 1080;
    var srcTags = makeTags(1280, 720, '420P', 0);
    var dstTags = makeTags(dstWidth, dstHeight, '420P', 0);
  
    var dstBufLen = stamper.setInfo(srcTags, dstTags);
    var numBytesExpected = dstWidth * dstHeight * 3/2;
    t.equal(dstBufLen, numBytesExpected, 'buffer size calculation matches the expected value');
    done();
  });

stampTest('Performing wipe of 420P', 2,
  (t, err) => t.notOk(err, 'no error expected'), 
  (t, stamper, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, '420P', 0);
    var dstTags = makeTags(width, height, '420P', 0);
    var dstBufLen = stamper.setInfo(srcTags, dstTags);
    var paramTags = { wipeRect:[0,0,1280,720], wipeCol:[1.0,0.0,0.0] };

    var dstBuf = Buffer.alloc(dstBufLen);
    stamper.wipe(dstBuf, paramTags, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = make420PBuf(width, height, { y:235, cb:128, cr:128 });
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');
      done();
    });
  });

stampTest('Performing wipe of YUV422P10', 2,
  (t, err) => t.notOk(err, 'no error expected'), 
  (t, stamper, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'YUV422P10', 0);
    var dstTags = makeTags(width, height, 'YUV422P10', 0);
    var dstBufLen = stamper.setInfo(srcTags, dstTags);
    var paramTags = { wipeRect:[0,0,1280,720], wipeCol:[1.0,0.0,0.0] };

    var dstBuf = Buffer.alloc(dstBufLen);
    stamper.wipe(dstBuf, paramTags, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = makeYUV422P10Buf(width, height, { y:940, cb:512, cr:512 });
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');
      done();
    });
  });

stampTest('Performing copy of 420P', 2,
  (t, err) => t.notOk(err, 'no error expected'), 
  (t, stamper, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, '420P', 0);
    var dstTags = makeTags(width, height, '420P', 0);
    var dstBufLen = stamper.setInfo(srcTags, dstTags);
    var paramTags = { dstOrg:[0,0] };

    var srcBufArray = new Array(1);
    var srcBuf = make420PBuf(width, height, { y:16, cb:128, cr:128 });
    srcBufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    stamper.copy(srcBufArray, dstBuf, paramTags, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = make420PBuf(width, height, { y:16, cb:128, cr:128 });
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');
      done();
    });
  });

stampTest('Performing copy of YUV422P10', 2,
  (t, err) => t.notOk(err, 'no error expected'), 
  (t, stamper, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'YUV422P10', 0);
    var dstTags = makeTags(width, height, 'YUV422P10', 0);
    var dstBufLen = stamper.setInfo(srcTags, dstTags);
    var paramTags = { dstOrg:[0,0] };

    var srcBufArray = new Array(1);
    var srcBuf = makeYUV422P10Buf(width, height, { y:64, cb:512, cr:512 });
    srcBufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    stamper.copy(srcBufArray, dstBuf, paramTags, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = makeYUV422P10Buf(width, height, { y:64, cb:512, cr:512 });
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');
      done();
    });
  });

stampTest('Performing mix of 420P', 2,
  (t, err) => t.notOk(err, 'no error expected'), 
  (t, stamper, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, '420P', 0);
    var dstTags = makeTags(width, height, '420P', 0);
    var dstBufLen = stamper.setInfo(srcTags, dstTags);
    var paramTags = { pressure:0.5 };

    var srcBufArray = new Array(2);
    var srcBufA = make420PBuf(width, height, { y:112, cb:112, cr:112 });
    var srcBufB = make420PBuf(width, height, { y:144, cb:144, cr:144 });
    srcBufArray[0] = srcBufA;
    srcBufArray[1] = srcBufB;
    var dstBuf = Buffer.alloc(dstBufLen);
    stamper.mix(srcBufArray, dstBuf, paramTags, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = make420PBuf(width, height, { y:128, cb:128, cr:128 });
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');
      done();
    });
  });

stampTest('Performing mix of YUV422P10', 2,
  (t, err) => t.notOk(err, 'no error expected'), 
  (t, stamper, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'YUV422P10', 0);
    var dstTags = makeTags(width, height, 'YUV422P10', 0);
    var dstBufLen = stamper.setInfo(srcTags, dstTags);
    var paramTags = { pressure:0.5 };

    var srcBufArray = new Array(2);
    var srcBufA = makeYUV422P10Buf(width, height, { y:448, cb:448, cr:448 });
    var srcBufB = makeYUV422P10Buf(width, height, { y:576, cb:576, cr:576 });
    srcBufArray[0] = srcBufA;
    srcBufArray[1] = srcBufB;
    var dstBuf = Buffer.alloc(dstBufLen);
    stamper.mix(srcBufArray, dstBuf, paramTags, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = makeYUV422P10Buf(width, height, { y:512, cb:512, cr:512 });
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');
      done();
    });
  });
