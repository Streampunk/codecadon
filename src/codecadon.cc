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

#include <nan.h>
#include "Concater.h"
#include "Flipper.h"
#include "Packer.h"
#include "ScaleConverter.h"
#include "Decoder.h"
#include "Encoder.h"
#include "Stamper.h"

using namespace v8;

NAN_MODULE_INIT(Init) {
  streampunk::Concater::Init(target);
  streampunk::Flipper::Init(target);
  streampunk::Packer::Init(target);
  streampunk::ScaleConverter::Init(target);
  streampunk::Decoder::Init(target);
  streampunk::Encoder::Init(target);
  streampunk::Stamper::Init(target);
}

NODE_MODULE(codecadon, Init)
