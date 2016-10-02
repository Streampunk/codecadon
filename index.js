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

'use strict';
var codecAdon = require('bindings')('./Release/codecadon');

//var SegfaultHandler = require('../node-segfault-handler');
//SegfaultHandler.registerHandler("crash.log");

const util = require('util');
const EventEmitter = require('events');

function Concater(cb) {
  this.concaterAdon = new codecAdon.Concater(cb);
  EventEmitter.call(this);
}

util.inherits(Concater, EventEmitter);

Concater.prototype.setInfo = function(srcTags) {
  try {
    return this.concaterAdon.setInfo(srcTags);
  } catch (err) {
    this.emit('error', err);
    return 0;
  }
}

Concater.prototype.concat = function(srcBufArray, dstBuf, cb) {
  try {
    var numQueued = this.concaterAdon.concat(srcBufArray, dstBuf, function(err, resultBytes) {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
}

Concater.prototype.quit = function(cb) {
  try {
    this.concaterAdon.quit(function(err, resultBytes) {
      cb(err, resultBytes);
    });
  } catch (err) {
    this.emit('error', err);
  }
}


function Packer(cb) {
  this.packerAdon = new codecAdon.Packer(cb);
  EventEmitter.call(this);
}

util.inherits(Packer, EventEmitter);

Packer.prototype.setInfo = function(srcTags, dstTags) {
  try {
    return this.packerAdon.setInfo(srcTags, dstTags);
  } catch (err) {
    this.emit('error', err);
    return 0;
  }
}

Packer.prototype.pack = function(srcBufArray, dstBuf, cb) {
  try {
    var numQueued = this.packerAdon.pack(srcBufArray, dstBuf, function(err, resultBytes) {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
}

Packer.prototype.quit = function(cb) {
  try {
    this.packerAdon.quit(function(err, resultBytes) {
      cb(err, resultBytes);
    });
  } catch (err) {
    this.emit('error', err);
  }
}


function ScaleConverter(cb) {
  this.scaleConverterAdon = new codecAdon.ScaleConverter(cb);
  EventEmitter.call(this);
}

util.inherits(ScaleConverter, EventEmitter);

ScaleConverter.prototype.setInfo = function(srcTags, dstTags, scaleTags) {
  var paramTags = { scale:[1.0, 1.0], dstOffset:[0.0, 0.0] };
  if (typeof scaleTags === 'object')
    paramTags = scaleTags;

  try {
    return this.scaleConverterAdon.setInfo(srcTags, dstTags, paramTags);
  } catch (err) {
    this.emit('error', err);
    return 0;
  }
}

ScaleConverter.prototype.scaleConvert = function(srcBufArray, dstBuf, cb) {
  try {
    var numQueued = this.scaleConverterAdon.scaleConvert(srcBufArray, dstBuf, function(err, resultBytes) {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
}

ScaleConverter.prototype.quit = function(cb) {
  try {
    this.scaleConverterAdon.quit(function(err, resultBytes) {
      cb(err, resultBytes);
    });
  } catch (err) {
    this.emit('error', err);
  }
}


function Decoder (cb) {
  this.decoderAdon = new codecAdon.Decoder(cb);
  EventEmitter.call(this);
}

util.inherits(Decoder, EventEmitter);

Decoder.prototype.setInfo = function(srcTags, dstTags) {
  try {
    return this.decoderAdon.setInfo(srcTags, dstTags);
  } catch (err) {
    this.emit('error', err);
    return 0;
  }
}

Decoder.prototype.decode = function(srcBufArray, dstBuf, cb) {
  try {
    var numQueued = this.decoderAdon.decode(srcBufArray, dstBuf, function(err, resultBytes) {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
}

Decoder.prototype.quit = function(cb) {
  try {
    this.decoderAdon.quit(function(err, resultBytes) {
      cb(err, resultBytes);
    });
  } catch (err) {
    this.emit('error', err);
  }
}


function Encoder (cb) {
  this.encoderAdon = new codecAdon.Encoder(cb);
  EventEmitter.call(this);
}

util.inherits(Encoder, EventEmitter);

Encoder.prototype.setInfo = function(srcTags, dstTags, duration, bitrate, gopframes) {
  var encodeBitrate = 5000000;
  if (typeof arguments[3] === 'number')
    encodeBitrate = arguments[3];

  var encodeGopFrames = 10 * 60;
  if (typeof arguments[4] === 'number')
    encodeGopFrames = arguments[4];

  try {
    return this.encoderAdon.setInfo(srcTags, dstTags, duration, encodeBitrate, encodeGopFrames);
  } catch (err) {
    this.emit('error', err);
    return 0;
  }
}

Encoder.prototype.encode = function(srcBufArray, dstBuf, cb) {
  try {
    var numQueued = this.encoderAdon.encode(srcBufArray, dstBuf, function(err, resultBytes) {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
}

Encoder.prototype.quit = function(cb) {
  try {
    this.encoderAdon.quit(function(err, resultBytes) {
      cb(err, resultBytes);
    });
  } catch (err) {
    this.emit('error', err);
  }
}


function Stamper(cb) {
  this.stamperAdon = new codecAdon.Stamper(cb);
  EventEmitter.call(this);
}

util.inherits(Stamper, EventEmitter);

Stamper.prototype.setInfo = function(srcTags, dstTags) {
  try {
    return this.stamperAdon.setInfo(srcTags, dstTags);
  } catch (err) {
    this.emit('error', err);
    return 0;
  }
}

Stamper.prototype.wipe = function(dstBuf, paramTags, cb) {
  try {
    var numQueued = this.stamperAdon.wipe(dstBuf, paramTags, function(err, resultBytes) {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
}

Stamper.prototype.copy = function(srcBufArray, dstBuf, paramTags, cb) {
  try {
    var numQueued = this.stamperAdon.copy(srcBufArray, dstBuf, paramTags, function(err, resultBytes) {
      cb(err, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
}

Stamper.prototype.quit = function(cb) {
  try {
    this.stamperAdon.quit(function(err, resultBytes) {
      cb(err, resultBytes);
    });
  } catch (err) {
    this.emit('error', err);
  }
}


var codecadon = {
   Concater : Concater,
   Packer : Packer,
   ScaleConverter : ScaleConverter,
   Decoder : Decoder,
   Encoder : Encoder,
   Stamper : Stamper
};

module.exports = codecadon;
