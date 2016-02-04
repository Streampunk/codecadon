#include <nan.h>
#include "Encoder.h"

using namespace v8;

NAN_MODULE_INIT(Init) {
  streampunk::Encoder::Init(target);
}

NODE_MODULE(codecadon, Init)
