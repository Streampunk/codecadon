#include <nan.h>
#include "iProcess.h"
using namespace v8;

namespace streampunk {

class MyWorker : public Nan::AsyncProgressWorker {
public:
  MyWorker (Nan::Callback *callback, iProcess *process)
    : Nan::AsyncProgressWorker(callback), mProcess(process)
    , mFrameCallback(NULL), mActive(true), mSrcBytes(0), mDstBuf(NULL), mDstBytes(0) {
    mDoWorkEvent = CreateEvent (NULL, /*Manual Reset*/FALSE, /*Initial State*/FALSE, NULL);
    mWorkerFreeEvent = CreateEvent (NULL, /*Manual Reset*/FALSE, /*Initial State*/TRUE, NULL);
  }

  ~MyWorker() {
    CloseHandle(mDoWorkEvent);
    CloseHandle(mWorkerFreeEvent);
  }

  void doFrame(Local<Array> srcBufArray, Nan::Callback* frameCallback) {
    Nan::HandleScope scope;
    WaitForSingleObject(mWorkerFreeEvent, INFINITE);
    SaveToPersistent("srcBuf", srcBufArray);

    mSrcBytes = 0;
    mSrcBufVec.clear();
    for (uint32_t i = 0; i < srcBufArray->Length(); ++i) {
      Local<Object> bufferObj = Local<Object>::Cast(srcBufArray->Get(i));
      uint32_t bufLen = (uint32_t)node::Buffer::Length(bufferObj);
      mSrcBufVec.push_back(std::make_pair(node::Buffer::Data(bufferObj), bufLen)); 
      mSrcBytes += bufLen;
    }

    Local<Object> dstBuf = Nan::NewBuffer(mSrcBytes).ToLocalChecked();
    Local<Array> dstBufArray = Nan::New<Array>(1);
    dstBufArray->Set(0, dstBuf);
    
    mDstBuf = node::Buffer::Data(dstBuf);
    SaveToPersistent("dstBuf", dstBufArray);
    mFrameCallback = frameCallback;

    SetEvent(mDoWorkEvent);
  }

  void quit() {
    WaitForSingleObject(mWorkerFreeEvent, INFINITE);
    mActive = false;    
    SetEvent(mDoWorkEvent);
  }

private:  
  void Execute(const ExecutionProgress& progress) {
  // Asynchronous, non-V8 work goes here
    while (mActive) {
      WaitForSingleObject(mDoWorkEvent, INFINITE);  
      if (!mActive)
        break;
      mProcess->processFrame (mSrcBufVec, mDstBuf);
      progress.Send(reinterpret_cast<const char*>(mDstBuf), mDstBytes);
    }
  }
  
  void HandleProgressCallback(const char *data, size_t size) {
    Nan::HandleScope scope;
    Local<Value> argv[] = { GetFromPersistent("dstBuf") };
    mFrameCallback->Call(1, argv);
    SetEvent(mWorkerFreeEvent);
  }
  
  void HandleOKCallback() {
    Nan::HandleScope scope;
    callback->Call(0, NULL);
  }

  iProcess* mProcess;
  Nan::Callback* mFrameCallback;
  bool mActive;
  tBufVec mSrcBufVec;
  uint32_t mSrcBytes;

  char* mDstBuf;
  uint32_t mDstBytes;

  HANDLE mDoWorkEvent;
  HANDLE mWorkerFreeEvent;
};

} // namespace streampunk
