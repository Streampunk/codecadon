#include <nan.h>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "iProcess.h"
using namespace v8;

namespace streampunk {

template <class T>
class WorkQueue {
public:
  WorkQueue() : qu(), m(), cv() {}
  ~WorkQueue() {}
  
  void enqueue(T t) {
    std::lock_guard<std::mutex> lk(m);
    qu.push(t);
    cv.notify_one();
  }
  
  T dequeue() {
    std::unique_lock<std::mutex> lk(m);
    while(qu.empty()) {
      cv.wait(lk);
    }
    T val = qu.front();
    qu.pop();
    return val;
  }

private:
  std::queue<T> qu;
  std::mutex m;
  std::condition_variable cv;
};

class MyWorker : public Nan::AsyncProgressWorker {
public:
  MyWorker (Nan::Callback *callback)
    : Nan::AsyncProgressWorker(callback), mActive(true) {}
  ~MyWorker() {}

  void doFrame(Local<Array> srcBufArray, iProcess *process, Local<Function> frameCallback) {
    uint32_t srcBytes = 0;
    tBufVec srcBufVec;
    for (uint32_t i = 0; i < srcBufArray->Length(); ++i) {
      Local<Object> bufferObj = Local<Object>::Cast(srcBufArray->Get(i));
      uint32_t bufLen = (uint32_t)node::Buffer::Length(bufferObj);
      srcBufVec.push_back(std::make_pair(node::Buffer::Data(bufferObj), bufLen)); 
      srcBytes += bufLen;
    }

    uint32_t dstBytes = srcBytes;
    Local<Object> dstBuf = Nan::NewBuffer(dstBytes).ToLocalChecked();
    Local<Array> dstBufArray = Nan::New<Array>(1);
    dstBufArray->Set(0, dstBuf);

    SaveToPersistent("callback", frameCallback);
    SaveToPersistent("srcBuf", srcBufArray);
    SaveToPersistent("dstBuf", dstBufArray);

    mWorkQueue.enqueue (
      new WorkParams(process, srcBufVec, srcBytes, node::Buffer::Data(dstBuf), dstBytes));
  }

  void quit() {
    mActive = false;    
    mWorkQueue.enqueue (new WorkParams(NULL, tBufVec(), 0, NULL, 0));
  }

private:  
  void Execute(const ExecutionProgress& progress) {
  // Asynchronous, non-V8 work goes here
    while (mActive) {
      WorkParams *p = mWorkQueue.dequeue();
      if (!(mActive && p->mProcess))
        break;
      p->mProcess->processFrame (p->mSrcBufVec, p->mDstBuf);
      progress.Send(NULL, 0);
      delete p;
    }
  }
  
  void HandleProgressCallback(const char *data, size_t size) {
    Nan::HandleScope scope;
    Local<Function> frameCallback = Local<Function>::Cast(GetFromPersistent("callback"));
    Local<Value> argv[] = { GetFromPersistent("dstBuf") };
    Nan::Callback(frameCallback).Call(1, argv);
  }
  
  void HandleOKCallback() {
    Nan::HandleScope scope;
    callback->Call(0, NULL);
  }

  bool mActive;
  struct WorkParams {
    WorkParams (iProcess *process, tBufVec srcBufVec, uint32_t srcBytes, char *dstBuf, uint32_t dstBytes) 
      : mProcess(process), mSrcBufVec(srcBufVec), mSrcBytes(srcBytes), mDstBuf(dstBuf), mDstBytes(dstBytes) {}

    iProcess *mProcess;
    tBufVec mSrcBufVec;
    uint32_t mSrcBytes;
    char *mDstBuf;
    uint32_t mDstBytes;
  };
  WorkQueue<WorkParams*> mWorkQueue;
};

} // namespace streampunk
