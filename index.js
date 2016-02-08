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
var codecAddon = require('bindings')('./release/codecadon');

const util = require('util');
const EventEmitter = require('events');

function Encoder (format) {
  if (arguments.length !== 1 || typeof format !== 'number' ) {
    this.emit('error', new Error('Encoder requires one argument: ' +
      'format'));
  } else {
    this.encoderAddon = new codecAddon.Encoder(format);
  }
  EventEmitter.call(this);
}

util.inherits(Encoder, EventEmitter);

Encoder.prototype.start = function() {
  try {
    this.encoderAddon.start(function() {
      this.emit('exit');
    }.bind(this));
  } catch (err) {
    this.emit('error', err);
  }
}

Encoder.prototype.encode = function (srcBufArray) {
  try {
    this.encoderAddon.encode(srcBufArray, function (result) {
      this.emit('encoded', result);
    }.bind(this));
  } catch (err) {
    this.emit('error', err);
  }
}

Encoder.prototype.quit = function (srcBufArray, cb) {
  try {
    this.encoderAddon.quit();
    this.emit('quit');
  } catch (err) {
    this.emit('error', err);
  }
}

var codecadon = {
   Encoder : Encoder
};

module.exports = codecadon;
