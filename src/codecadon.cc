#include <nan.h>
#include "Concater.h"
#include "ScaleConverter.h"
#include "Encoder.h"

using namespace v8;

NAN_MODULE_INIT(Init) {
  streampunk::Concater::Init(target);
  streampunk::ScaleConverter::Init(target);
  streampunk::Encoder::Init(target);
}

NODE_MODULE(codecadon, Init)
