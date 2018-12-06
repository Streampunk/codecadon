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

'use strict';
var codecAdon = require('bindings')('./Release/codecadon');

var SegfaultHandler = require('segfault-handler');
SegfaultHandler.registerHandler('crash.log');

const util = require('util');
const EventEmitter = require('events');

function Concater(cb) {
  this.concaterAdon = new codecAdon.Concater(cb);
  EventEmitter.call(this);
}

util.inherits(Concater, EventEmitter);

Concater.prototype.setInfo = function(srcTags, logLevel) {
  let debugLevel = (typeof logLevel === 'number')?logLevel:3;
  try {
    return this.concaterAdon.setInfo(srcTags, debugLevel);
  } catch (err) {
    this.emit('error', err);
    return 0;
  }
};

Concater.prototype.concat = function(srcBufArray, dstBuf, cb) {
  try {
    var numQueued = this.concaterAdon.concat(srcBufArray, dstBuf, (err, resultBytes) => {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
};

Concater.prototype.quit = function(cb) {
  try {
    this.concaterAdon.quit((err, resultBytes) => {
      cb(err, resultBytes);
    });
  } catch (err) {
    this.emit('error', err);
  }
};


function Flipper(cb) {
  this.flipperAdon = new codecAdon.Flipper(cb);
  EventEmitter.call(this);
}

util.inherits(Flipper, EventEmitter);

Flipper.prototype.setInfo = function(srcTags, flip, logLevel) {
  let debugLevel = (typeof logLevel === 'number')?logLevel:3;
  try {
    return this.flipperAdon.setInfo(srcTags, flip, debugLevel);
  } catch (err) {
    this.emit('error', err);
    return 0;
  }
};

Flipper.prototype.flip = function(srcBufArray, dstBuf, cb) {
  try {
    var numQueued = this.flipperAdon.flip(srcBufArray, dstBuf, (err, resultBytes) => {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
};

Flipper.prototype.quit = function(cb) {
  try {
    this.flipperAdon.quit((err, resultBytes) => {
      cb(err, resultBytes);
    });
  } catch (err) {
    this.emit('error', err);
  }
};


function Packer(cb) {
  this.packerAdon = new codecAdon.Packer(cb);
  EventEmitter.call(this);
}

util.inherits(Packer, EventEmitter);

Packer.prototype.setInfo = function(srcTags, dstTags, logLevel) {
  let debugLevel = (typeof logLevel === 'number')?logLevel:3;
  try {
    return this.packerAdon.setInfo(srcTags, dstTags, debugLevel);
  } catch (err) {
    this.emit('error', err);
    return 0;
  }
};

Packer.prototype.pack = function(srcBufArray, dstBuf, cb) {
  try {
    var numQueued = this.packerAdon.pack(srcBufArray, dstBuf, (err, resultBytes) => {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
};

Packer.prototype.quit = function(cb) {
  try {
    this.packerAdon.quit((err, resultBytes) => {
      cb(err, resultBytes);
    });
  } catch (err) {
    this.emit('error', err);
  }
};


function ScaleConverter(cb) {
  this.scaleConverterAdon = new codecAdon.ScaleConverter(cb);
  EventEmitter.call(this);
}

util.inherits(ScaleConverter, EventEmitter);

ScaleConverter.prototype.setInfo = function(srcTags, dstTags, scaleTags, logLevel) {
  let debugLevel = (typeof logLevel === 'number')?logLevel:3;
  var paramTags = { scale:[1.0, 1.0], dstOffset:[0.0, 0.0] };
  if (typeof scaleTags === 'object')
    paramTags = scaleTags;

  try {
    return this.scaleConverterAdon.setInfo(srcTags, dstTags, paramTags, debugLevel);
  } catch (err) {
    this.emit('error', err);
    return 0;
  }
};

ScaleConverter.prototype.scaleConvert = function(srcBufArray, dstBuf, cb) {
  try {
    var numQueued = this.scaleConverterAdon.scaleConvert(srcBufArray, dstBuf, (err, resultBytes) => {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
};

ScaleConverter.prototype.quit = function(cb) {
  try {
    this.scaleConverterAdon.quit((err, resultBytes) => {
      cb(err, resultBytes);
    });
  } catch (err) {
    this.emit('error', err);
  }
};


function Decoder (cb) {
  this.decoderAdon = new codecAdon.Decoder(cb);
  EventEmitter.call(this);
}

util.inherits(Decoder, EventEmitter);

Decoder.prototype.setInfo = function(srcTags, dstTags, logLevel) {
  let debugLevel = (typeof logLevel === 'number')?logLevel:3;
  try {
    return this.decoderAdon.setInfo(srcTags, dstTags, debugLevel);
  } catch (err) {
    this.emit('error', err);
    return 0;
  }
};

Decoder.prototype.decode = function(srcBufArray, dstBuf, cb) {
  try {
    var numQueued = this.decoderAdon.decode(srcBufArray, dstBuf, (err, resultBytes) => {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
};

Decoder.prototype.quit = function(cb) {
  try {
    this.decoderAdon.quit((err, resultBytes) => {
      cb(err, resultBytes);
    });
  } catch (err) {
    this.emit('error', err);
  }
};


function Encoder (cb) {
  this.encoderAdon = new codecAdon.Encoder(cb);
  EventEmitter.call(this);
}

util.inherits(Encoder, EventEmitter);

Encoder.prototype.setInfo = function(srcTags, dstTags, duration, encodeTags, logLevel) {
  let debugLevel = (typeof logLevel === 'number')?logLevel:3;
  try {
    return this.encoderAdon.setInfo(srcTags, dstTags, duration, encodeTags, debugLevel);
  } catch (err) {
    this.emit('error', err);
    return 0;
  }
};

Encoder.prototype.encode = function(srcBufArray, dstBuf, cb) {
  try {
    var numQueued = this.encoderAdon.encode(srcBufArray, dstBuf, (err, resultBytes) => {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
};

Encoder.prototype.quit = function(cb) {
  try {
    this.encoderAdon.quit((err, resultBytes) => {
      cb(err, resultBytes);
    });
  } catch (err) {
    this.emit('error', err);
  }
};


function Stamper(cb) {
  this.stamperAdon = new codecAdon.Stamper(cb);
  EventEmitter.call(this);
}

util.inherits(Stamper, EventEmitter);

Stamper.prototype.setInfo = function(srcTags, dstTags, logLevel) {
  let debugLevel = (typeof logLevel === 'number')?logLevel:3;
  var srcTagsArray = [];
  if (Array.isArray(srcTags))
    srcTagsArray = srcTags;
  else
    srcTagsArray = [ srcTags ];

  try {
    return this.stamperAdon.setInfo(srcTagsArray, dstTags, debugLevel);
  } catch (err) {
    this.emit('error', err);
    return 0;
  }
};

Stamper.prototype.wipe = function(dstBuf, paramTags, cb) {
  try {
    var numQueued = this.stamperAdon.wipe(dstBuf, paramTags, (err, resultBytes) => {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
};

Stamper.prototype.copy = function(srcBufArray, dstBuf, paramTags, cb) {
  try {
    var numQueued = this.stamperAdon.copy(srcBufArray, dstBuf, paramTags, (err, resultBytes) => {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
};

Stamper.prototype.mix = function(srcBufArray, dstBuf, paramTags, cb) {
  try {
    var numQueued = this.stamperAdon.mix(srcBufArray, dstBuf, paramTags, (err, resultBytes) => {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
};

Stamper.prototype.stamp = function(srcBufArray, dstBuf, paramTags, cb) {
  try {
    var numQueued = this.stamperAdon.stamp(srcBufArray, dstBuf, paramTags, (err, resultBytes) => {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
};

Stamper.prototype.quit = function(cb) {
  try {
    this.stamperAdon.quit((err, resultBytes) => {
      cb(err, resultBytes);
    });
  } catch (err) {
    this.emit('error', err);
  }
};


var codecadon = {
  Concater : Concater,
  Flipper : Flipper,
  Packer : Packer,
  ScaleConverter : ScaleConverter,
  Decoder : Decoder,
  Encoder : Encoder,
  Stamper : Stamper
};

module.exports = codecadon;
