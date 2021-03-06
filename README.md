[![CircleCI](https://circleci.com/gh/Streampunk/codecadon.svg?style=shield&circle-token=:circle-token)](https://circleci.com/gh/Streampunk/codecadon)
[![npm version](https://badge.fury.io/js/codecadon.svg)](https://badge.fury.io/js/codecadon)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
# Codecadon

Codecadon is a [Node.js](http://nodejs.org/) [addon](http://nodejs.org/api/addons.html) using Javascript and C++ to implement async processing for media encoding, decoding and processing.

The implementation is designed to support the [dynamorse](http://github.com/Streampunk/dynamorse) project and currently provides packing, rescaling and encoding/decoding for h.264 and VP8, largely using [FFmpeg](http://www.ffmpeg.org/).

Support has been added for building codecadon for the Raspberry Pi armhf platforms, although do not expect anything other than relatively poor performance at this stage.

## Installation
[![NPM](https://nodei.co/npm/codecadon.png?downloads=true)](https://www.npmjs.com/package/codecadon)

Install [Node.js](http://nodejs.org/) for your platform. This software has been developed against the long term stable (LTS) release.

Codecadon is designed to be `require`d to use from your own application to provide async processing. Install in your project folder with the --save option to add codecadon to the dependencies list:

    npm install --save codecadon

Node.js addons use [libuv](http://libuv.org/) which by default supports up to 4 async threads in a threadpool for activities such as file I/O. These same threads are used by codecadon and if you wish to use a number of the functions in one Node.js process then you will need to set the environment variable UV_THREADPOOL_SIZE to a suitable number before the first use of the libuv threadpool.

Example shell commands to set this variable on different platforms are:

Windows:

    set UV_THREADPOOL_SIZE=16

Linux/Mac/Raspberry Pi:

    export UV_THREADPOOL_SIZE=16

## Using codecadon

To use codecadon in your own application, `require` the module then create and use workers as required.  The processing functions follow a standard pattern as shown in the encoder example code below.

```javascript
let codecadon = require('codecadon');

let encoder = new codecadon.Encoder(() => {
  // encoder has successfully exited
});
encoder.on('error', err => {
  // handle error
});

// send the parameters for the function (these differ by function), return value is suggested result buffer size
let dstBufLen = encoder.setInfo(srcTags, dstTags, duration, encodeTags);

// async request for processing to be done.
encoder.encode(srcBuf, Buffer.alloc(dstBufLen), (err, result) => {
  if (err) {
    // handle error
  } else if (result) {
    // result is a JS buffer containing the encoded data. Note it may be a different size to dstBuf
  }
});

// async request for the processing thread to quit when it has finished the latest request
encoder.quit(() => {
});
```

## Status, support and further development

There is currently a limited set of video packing formats and codecs supported.  There has been no attempt made to tune encoder parameters for performance or quality.

Although the architecture of codecadon is such that it could be used at scale in production environments, development is not yet complete. In its current state, it is recommended that this software is used in development environments and for building prototypes. Future development will make this more appropriate for production use.

Contributions can be made via pull requests and will be considered by the author on their merits. Enhancement requests and bug reports should be raised as github issues. For support, please contact [Streampunk Media](http://www.streampunk.media/).

## License

This software is released under the Apache 2.0 license. Copyright 2016 Streampunk Media Ltd.

This software uses libraries from the FFmpeg project under the LGPLv3.

Copies of the licenses pertaining to libraries used in this software can be found in the licences folder.
