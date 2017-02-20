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

#ifndef ENCODERFACTORY_H
#define ENCODERFACTORY_H

#include <memory>
#include "EssenceInfo.h"
#include "EncoderFF.h"
#include "EncodeParams.h"

namespace streampunk {

class Duration;

class EncoderFactory {
public:
  static std::shared_ptr<iEncoderDriver> createEncoder(std::shared_ptr<EssenceInfo> srcInfo, std::shared_ptr<EssenceInfo> dstInfo, 
                                                       const Duration& duration, std::shared_ptr<EncodeParams> encodeParams) {
    if ((0 == dstInfo->packing().compare("AVCi50")) || (0 == dstInfo->packing().compare("AVCi100"))) {   
      throw std::runtime_error("No implementation of AVCi encoder available");
    }
    else {
      return std::make_shared<EncoderFF>(srcInfo, dstInfo, duration, encodeParams);
    }
  }
};

} // namespace streampunk

#endif
