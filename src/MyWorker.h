/* Copyright 2016 Christine S. MacNeill

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

#ifndef MYWORKER_H
#define MYWORKER_H

#include <nan.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

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
        wp->mResultBytes = wp->mProcess->processFrame(wp->mProcessData);
      else
        mActive = false;
      mDoneQueue.enqueue(wp);
      progress.Send(NULL, 0);
    }

    // wait for quit message to be passed to callback
    std::unique_lock<std::mutex> lk(mMtx);
    mCv.wait(lk);
  }
  
  void HandleProgressCallback(const char *data, size_t size) {
    Nan::HandleScope scope;
    while (mDoneQueue.size() != 0)
    {
      std::shared_ptr<WorkParams> wp = mDoneQueue.dequeue();
      Local<Value> argv[] = { Nan::Null(), Nan::New(wp->mResultBytes) };
      wp->mCallback->Call(2, argv);

      if (!wp->mProcess && !mActive) {
        // notify the thread to exit
        std::unique_lock<std::mutex> lk(mMtx);
        mCv.notify_one();
      }
    }
  }
  
  void HandleOKCallback() {
    Nan::HandleScope scope;
    callback->Call(0, NULL);
  }

  bool mActive;
  struct WorkParams {
    WorkParams(std::shared_ptr<iProcessData> processData, iProcess *process, Nan::Callback *callback)
      : mProcessData(processData), mProcess(process), mCallback(callback), mResultBytes(0) {}
    ~WorkParams() { delete mCallback; }

    std::shared_ptr<iProcessData> mProcessData;
    iProcess *mProcess;
    Nan::Callback *mCallback;
    uint32_t mResultBytes;
  };
  WorkQueue<std::shared_ptr<WorkParams> > mWorkQueue;
  WorkQueue<std::shared_ptr<WorkParams> > mDoneQueue;
  std::mutex mMtx;
  std::condition_variable mCv;
};

} // namespace streampunk

#endif
