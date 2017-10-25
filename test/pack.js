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

function makeUYVY10Buf(width, height) {
  var pitchBytes = width * 4;
  var buf = Buffer.alloc(pitchBytes * height);  
  var yOff = 0;
  for (var y=0; y<height; ++y) {
    var xOff = 0;
    for (var x=0; x<width; x+=2) {
      buf.writeUInt16LE(0x0200, yOff + xOff);
      buf.writeUInt16LE(0x0040, yOff + xOff + 2);
      buf.writeUInt16LE(0x0200, yOff + xOff + 4);
      buf.writeUInt16LE(0x0040, yOff + xOff + 6);
      xOff += 8;
    }
    yOff += pitchBytes;
  }   
  return buf;
}

function makeV210Buf(width, height) {
  var pitchBytes = (width + (47 - (width - 1) % 48)) * 8 / 3;
  var buf = Buffer.alloc(pitchBytes * height);
  buf.fill(0);
  var yOff = 0;
  for (var y=0; y<height; ++y) {
    var xOff = 0;
    for (var x=0; x<(width-width%6)/6; ++x) {
      buf.writeUInt32LE((0x200<<20) | (0x040<<10) | 0x200, yOff + xOff);
      buf.writeUInt32LE((0x040<<20) | (0x200<<10) | 0x040, yOff + xOff + 4);
      buf.writeUInt32LE((0x200<<20) | (0x040<<10) | 0x200, yOff + xOff + 8);
      buf.writeUInt32LE((0x040<<20) | (0x200<<10) | 0x040, yOff + xOff + 12);
      xOff += 16;
    }

    var remain = width%6;
    if (remain) {
      buf.writeUInt32LE((0x200<<20) | (0x040<<10) | 0x200, yOff + xOff);
      if (2 === remain) {
        buf.writeUInt32LE(0x040, yOff + xOff + 4);
      } else if (4 === remain) {      
        buf.writeUInt32LE((0x040<<20) | (0x200<<10) | 0x040, yOff + xOff + 4);
        buf.writeUInt32LE((0x040<<10) | 0x200, yOff + xOff + 8);
      }
    }
    yOff += pitchBytes;
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

function packTest(description, onErr, fn) {
  test(description, (t) => {
    var packer = new codecadon.Packer((() => {
      t.end();
    }));
    packer.on('error', (err) => {
      onErr(t, err);
    });

    fn(t, packer, () => {
      packer.quit(() => {});
    });
  });
}

packTest('Handling bad image dimensions', 
  (t, err) => {
    t.ok(err, 'emits error');
  }, 
  (t, packer, done) => {
    var srcTags = makeTags(1280, 720, 'pgroup', 0);
    var dstTags = makeTags(21, 0, '420P', 0);
    packer.setInfo(srcTags, dstTags);
    done();
  });

packTest('Handling bad image format',
  (t, err) => {
    t.ok(err, 'emits error');
  }, 
  (t, packer, done) => {
    var srcTags = makeTags(1280, 720, 'pgroup', 0);
    var dstTags = makeTags(1920, 1080, 'pgroup', 0);
    packer.setInfo(srcTags, dstTags);
    done();
  });

packTest('Starting up a packer', 
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var dstWidth = 1920;
    var dstHeight = 1080;
    var srcTags = makeTags(1280, 720, 'pgroup', 0);
    var dstTags = makeTags(dstWidth, dstHeight, '420P', 0);
  
    var dstBufLen = packer.setInfo(srcTags, dstTags);
    var numBytesExpected = dstWidth * dstHeight * 3/2;
    t.equal(dstBufLen, numBytesExpected, 'buffer size calculation matches the expected value');
    done();
  });

packTest('Performing packing pgroup to 420P',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'pgroup', 0);
    var dstTags = makeTags(width, height, '420P', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);

    var bufArray = new Array(1);
    var srcBuf = make4175Buf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    packer.pack(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = make420PBuf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');   
      done();
    });
  });

packTest('Performing packing V210 to 420P',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'v210', 0);
    var dstTags = makeTags(width, height, '420P', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);

    var bufArray = new Array(1);
    var srcBuf = makeV210Buf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    packer.pack(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = make420PBuf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');   
      done();
    });
  });

packTest('Performing packing pgroup to YUV422P10',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'pgroup', 0);
    var dstTags = makeTags(width, height, 'YUV422P10', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);

    var bufArray = new Array(1);
    var srcBuf = make4175Buf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    packer.pack(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = makeYUV422P10Buf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');   
      done();
    });
  });

packTest('Performing packing pgroup to UYVY10',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'pgroup', 0);
    var dstTags = makeTags(width, height, 'UYVY10', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);

    var bufArray = new Array(1);
    var srcBuf = make4175Buf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    packer.pack(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = makeUYVY10Buf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');   
      done();
    });
  });

packTest('Performing packing YUV422P10 to UYVY10',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'YUV422P10', 0);
    var dstTags = makeTags(width, height, 'UYVY10', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);

    var bufArray = new Array(1);
    var srcBuf = makeYUV422P10Buf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    packer.pack(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = makeUYVY10Buf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');   
      done();
    });
  });

packTest('Performing packing V210 to YUV422P10',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'v210', 0);
    var dstTags = makeTags(width, height, 'YUV422P10', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);

    var bufArray = new Array(1);
    var srcBuf = makeV210Buf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    packer.pack(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = makeYUV422P10Buf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');   
      done();
    });
  });

packTest('Performing packing UYVY10 to pgroup',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'UYVY10', 0);
    var dstTags = makeTags(width, height, 'pgroup', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);

    var bufArray = new Array(1);
    var srcBuf = makeUYVY10Buf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    packer.pack(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = make4175Buf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');   
      done();
    });
  });

packTest('Performing packing UYVY10 to YUV422P10',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'UYVY10', 0);
    var dstTags = makeTags(width, height, 'YUV422P10', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);

    var bufArray = new Array(1);
    var srcBuf = makeUYVY10Buf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    packer.pack(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = makeYUV422P10Buf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');   
      done();
    });
  });

packTest('Performing packing UYVY10 to 420P',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'UYVY10', 0);
    var dstTags = makeTags(width, height, '420P', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);

    var bufArray = new Array(1);
    var srcBuf = makeUYVY10Buf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    packer.pack(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = make420PBuf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');   
      done();
    });
  });

packTest('Performing packing YUV422P10 to 420P',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'YUV422P10', 0);
    var dstTags = makeTags(width, height, '420P', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);

    var bufArray = new Array(1);
    var srcBuf = makeYUV422P10Buf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    packer.pack(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = make420PBuf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');   
      done();
    });
  });

packTest('Performing packing YUV422P10 to pgroup',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'YUV422P10', 0);
    var dstTags = makeTags(width, height, 'pgroup', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);

    var bufArray = new Array(1);
    var srcBuf = makeYUV422P10Buf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    packer.pack(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = make4175Buf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');   
      done();
    });
  });

packTest('Performing packing 420P to pgroup',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, '420P', 0);
    var dstTags = makeTags(width, height, 'pgroup', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);

    var bufArray = new Array(1);
    var srcBuf = make420PBuf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    packer.pack(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = make4175Buf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');   
      done();
    });
  });

packTest('Performing packing YUV422P10 to V210',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'YUV422P10', 0);
    var dstTags = makeTags(width, height, 'v210', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);

    var bufArray = new Array(1);
    var srcBuf = makeYUV422P10Buf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    dstBuf.fill(0);
    packer.pack(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = makeV210Buf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');   
      done();
    });
  });

packTest('Performing packing 420P to V210',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, '420P', 0);
    var dstTags = makeTags(width, height, 'v210', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);

    var bufArray = new Array(1);
    var srcBuf = make420PBuf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    dstBuf.fill(0);
    packer.pack(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = makeV210Buf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');   
      done();
    });
  });

packTest('Performing packing pgroup to V210',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'pgroup', 0);
    var dstTags = makeTags(width, height, 'v210', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);

    var bufArray = new Array(1);
    var srcBuf = make4175Buf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    dstBuf.fill(0);
    packer.pack(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = makeV210Buf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');   
      done();
    });
  });

packTest('Performing packing V210 to pgroup',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1280;
    var height = 720;
    var srcTags = makeTags(width, height, 'v210', 0);
    var dstTags = makeTags(width, height, 'pgroup', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);

    var bufArray = new Array(1);
    var srcBuf = makeV210Buf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen);
    dstBuf.fill(0);
    packer.pack(bufArray, dstBuf, (err, result) => {
      t.notOk(err, 'no error expected');
      var testDstBuf = make4175Buf(width, height);
      t.deepEquals(result, testDstBuf, 'matches the expected packing result');   
      done();
    });
  });

packTest('Handling undefined source',
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var bufArray;
    var width = 1920;
    var height = 1080;
    var srcTags = makeTags(width, height, 'pgroup', 0);
    var dstTags = makeTags(width, height, '420P', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);
    var dstBuf = Buffer.alloc(dstBufLen);
    packer.pack(bufArray, dstBuf, (err/*, result*/) => {
      t.ok(err, 'should return error');
      done();
    });
  });

packTest('Handling undefined destination', 
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1920;
    var height = 1080;
    var srcTags = makeTags(width, height, 'pgroup', 0);
    var dstTags = makeTags(width, height, '420P', 0);
    /*var dstBufLen =*/ packer.setInfo(srcTags, dstTags);
    var bufArray = new Array(1);
    var srcBuf = make4175Buf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf;
    packer.pack(bufArray, dstBuf, (err/*, result*/) => {
      t.ok(err, 'should return error');
      done();
    });
  });

packTest('Handling insufficient destination bytes', 
  (t, err) => {
    t.notOk(err, 'no error expected');
  }, 
  (t, packer, done) => {
    var width = 1920;
    var height = 1080;
    var srcTags = makeTags(width, height, 'pgroup', 0);
    var dstTags = makeTags(width, height, '420P', 0);
    var dstBufLen = packer.setInfo(srcTags, dstTags);
    var bufArray = new Array(1);
    var srcBuf = make4175Buf(width, height);
    bufArray[0] = srcBuf;
    var dstBuf = Buffer.alloc(dstBufLen - 128);
    packer.pack(bufArray, dstBuf, (err/*, result*/) => {
      t.ok(err, 'should return error');
      done();
    });
  });
