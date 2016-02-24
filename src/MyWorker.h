#include <nan.h>
#include <queue>
#include <mutex>
#include <condition_variable>

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

class iProcess;
class iProcessData;
class MyWorker : public Nan::AsyncProgressWorker {
public:
  MyWorker (Nan::Callback *callback)
    : Nan::AsyncProgressWorker(callback), mActive(true) {}
  ~MyWorker() {}

  uint32_t numQueued() {
    return (uint32_t)mWorkQueue.size();
  }

  void doFrame(std::shared_ptr<iProcessData> processData, iProcess *process, Nan::Callback *frameCallback) {
    mWorkQueue.enqueue (
      std::make_shared<WorkParams>(processData, process, frameCallback));
  }

  void quit(Nan::Callback *callback) {
    mWorkQueue.enqueue (std::make_shared<WorkParams>(std::shared_ptr<iProcessData>(), (iProcess *)NULL, callback));
  }

private:  
  void Execute(const ExecutionProgress& progress) {
  // Asynchronous, non-V8 work goes here
    while (mActive) {
      std::shared_ptr<WorkParams> wp = mWorkQueue.dequeue();
      if (wp->mProcess)
        wp->mResultReady = wp->mProcess->processFrame(wp->mProcessData);
      else
        mActive = false;
      mDoneQueue.enqueue(wp);
      progress.Send(NULL, 0);
    }
  }
  
  void HandleProgressCallback(const char *data, size_t size) {
    Nan::HandleScope scope;
    while (mDoneQueue.size() != 0)
    {
      std::shared_ptr<WorkParams> wp = mDoneQueue.dequeue();
      Local<Value> argv[] = { Nan::New(wp->mResultReady) };
      wp->mCallback->Call(1, argv);
    }
  }
  
  void HandleOKCallback() {
    Nan::HandleScope scope;
    callback->Call(0, NULL);
  }

  bool mActive;
  struct WorkParams {
    WorkParams(std::shared_ptr<iProcessData> processData, iProcess *process, Nan::Callback *callback)
      : mProcessData(processData), mProcess(process), mCallback(callback), mResultReady(false) {}

    std::shared_ptr<iProcessData> mProcessData;
    iProcess *mProcess;
    Nan::Callback *mCallback;
    bool mResultReady;
  };
  WorkQueue<std::shared_ptr<WorkParams> > mWorkQueue;
  WorkQueue<std::shared_ptr<WorkParams> > mDoneQueue;
};

} // namespace streampunk
