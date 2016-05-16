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


function ScaleConverter(cb) {
  this.scaleConverterAdon = new codecAdon.ScaleConverter(cb);
  EventEmitter.call(this);
}

util.inherits(ScaleConverter, EventEmitter);

ScaleConverter.prototype.setInfo = function(srcTags, dstTags) {
  try {
    return this.scaleConverterAdon.setInfo(srcTags, dstTags);
  } catch (err) {
    this.emit('error', err);
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

Encoder.prototype.setInfo = function(srcTags, dstTags) {
  try {
    return this.encoderAdon.setInfo(srcTags, dstTags);
  } catch (err) {
    this.emit('error', err);
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

var codecadon = {
   Concater : Concater,
   ScaleConverter : ScaleConverter,
   Decoder : Decoder,
   Encoder : Encoder
};

module.exports = codecadon;
