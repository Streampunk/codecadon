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

'use strict';
var codecAdon = require('bindings')('./Release/codecadon');

//var SegfaultHandler = require('../node-segfault-handler');
//SegfaultHandler.registerHandler("crash.log");

const util = require('util');
const EventEmitter = require('events');

function Concater(numBytes) {
  this.concaterAdon = new codecAdon.Concater(numBytes);
  EventEmitter.call(this);
}

util.inherits(Concater, EventEmitter);

Concater.prototype.start = function() {
  try {
    return this.concaterAdon.start(function() {
      this.emit('exit');
    }.bind(this));
  } catch (err) {
    this.emit('error', err);
  }
}

Concater.prototype.concat = function(srcBufArray, dstBuf, cb) {
  try {
    var numQueued = this.concaterAdon.concat(srcBufArray, dstBuf, function(resultBytes) {
      cb(null, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
}

Concater.prototype.quit = function(cb) {
  try {
    this.concaterAdon.quit(function(done) {
      cb();
    });
  } catch (err) {
    this.emit('error', err);
  }
}

Concater.prototype.finish = function() {
  this.concaterAdon.finish();
}

function ScaleConverter(format, width, height) {
  if (arguments.length !== 3 
    || typeof format !== 'string' 
    || typeof width !== 'number' 
    || typeof height !== 'number') {
    this.emit('error', new Error('ScaleConverter requires three arguments: ' +
      format));
  } else {
    this.scaleConverterAdon = new codecAdon.ScaleConverter(format, width, height);
  }
  EventEmitter.call(this);
}

util.inherits(ScaleConverter, EventEmitter);

ScaleConverter.prototype.start = function() {
  try {
    return this.scaleConverterAdon.start(function() {
      this.emit('exit');
    }.bind(this));
  } catch (err) {
    this.emit('error', err);
  }
}

ScaleConverter.prototype.scaleConvert = function(srcBufArray, srcWidth, srcHeight, srcFmtCode, dstBuf, cb) {
  try {
    var numQueued = this.scaleConverterAdon.scaleConvert(srcBufArray, srcWidth, srcHeight, srcFmtCode, 
                                                         dstBuf, function(resultBytes) {
      cb(null, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
}

ScaleConverter.prototype.quit = function(cb) {
  try {
    this.scaleConverterAdon.quit(function(done) {
      cb();
    });
  } catch (err) {
    this.emit('error', err);
  }
}

ScaleConverter.prototype.finish = function() {
  this.scaleConverterAdon.finish();
}

function Encoder (format, width, height) {
  if (arguments.length !== 3 
    || typeof format !== 'string' 
    || typeof width !== 'number' 
    || typeof height !== 'number') {
    this.emit('error', new Error('Encoder requires three arguments: ' +
      format));
  } else {
    this.encoderAdon = new codecAdon.Encoder(format, width, height);
  }
  EventEmitter.call(this);
}

util.inherits(Encoder, EventEmitter);

Encoder.prototype.start = function() {
  try {
    return this.encoderAdon.start(function() {
      this.emit('exit');
    }.bind(this));
  } catch (err) {
    this.emit('error', err);
  }
}

Encoder.prototype.encode = function(srcBufArray, srcWidth, srcHeight, srcFmtCode, dstBuf, cb) {
  try {
    var numQueued = this.encoderAdon.encode(srcBufArray, srcWidth, srcHeight, srcFmtCode, 
                                            dstBuf, function(resultBytes) {
      cb(null, resultBytes?dstBuf.slice(0,resultBytes):null);
    });
    return numQueued;
  } catch (err) {
    cb(err);
  }
}

Encoder.prototype.quit = function(cb) {
  try {
    this.encoderAdon.quit(function(done) {
      cb();
    });
  } catch (err) {
    this.emit('error', err);
  }
}

Encoder.prototype.finish = function() {
  this.encoderAdon.finish();
}

var codecadon = {
   Concater : Concater,
   ScaleConverter : ScaleConverter,
   Encoder : Encoder
};

module.exports = codecadon;
