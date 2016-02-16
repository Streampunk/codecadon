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

  size_t size() {
    std::lock_guard<std::mutex> lk(m);
    return qu.size();
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

  uint32_t numQueued() {
    return (uint32_t)mWorkQueue.size();
  }

  void doFrame(iProcessData *processData, iProcess *process, Local<Function> frameCallback) {
    mWorkQueue.enqueue (
      new WorkParams(processData, process, new Nan::Callback(frameCallback)));
  }

  void quit() {
    mActive = false;    
    mWorkQueue.enqueue (new WorkParams(NULL, NULL, NULL));
  }

private:  
  void Execute(const ExecutionProgress& progress) {
  // Asynchronous, non-V8 work goes here
    while (mActive) {
      WorkParams *wp = mWorkQueue.dequeue();
      if (!(mActive && wp->mProcess))
        break;
      wp->mResultReady = wp->mProcess->processFrame (wp->mProcessData);
      mDoneQueue.enqueue (wp);
      progress.Send(NULL, 0);
    }
  }
  
  void HandleProgressCallback(const char *data, size_t size) {
    Nan::HandleScope scope;
    while (mDoneQueue.size() != 0)
    {
      WorkParams *wp = mDoneQueue.dequeue();
      Local<Value> argv[] = { Nan::New(wp->mResultReady) };
      wp->mFrameCallback->Call(1, argv);
      delete wp;
    }
  }
  
  void HandleOKCallback() {
    Nan::HandleScope scope;
    callback->Call(0, NULL);
  }

  bool mActive;
  struct WorkParams {
    WorkParams (iProcessData *processData, iProcess *process, Nan::Callback *frameCallback)
      : mProcessData(processData), mProcess(process), mFrameCallback(frameCallback), mResultReady(false) {}

    iProcessData *mProcessData;
    iProcess *mProcess;
    Nan::Callback *mFrameCallback;
    bool mResultReady;
  };
  WorkQueue<WorkParams*> mWorkQueue;
  WorkQueue<WorkParams*> mDoneQueue;
};

} // namespace streampunk
