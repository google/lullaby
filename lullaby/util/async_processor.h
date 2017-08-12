/*
Copyright 2017 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef LULLABY_UTIL_ASYNC_PROCESSOR_H_
#define LULLABY_UTIL_ASYNC_PROCESSOR_H_

#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "lullaby/util/thread_safe_deque.h"

namespace lull {

// The AsyncProcessor is used for performing async operations on objects of type
// |T| using worker threads.
template <typename T>
class AsyncProcessor {
 public:
  // The function to be called on the object on the worker thread.
  using ProcessFn = std::function<void(T*)>;

  using TaskId = unsigned int;

  enum : TaskId { kInvalidTaskId = 0 };

  // Creates the AsyncProcessor with the specified number of worker threads.
  explicit AsyncProcessor(size_t num_worker_threads = 1);

  AsyncProcessor(const AsyncProcessor& rhs) = delete;
  AsyncProcessor& operator=(const AsyncProcessor& rhs) = delete;

  // Waits for all worker threads to complete processing requests.
  ~AsyncProcessor();

  // Starts the worker threads. This is done automatically on construction, and
  // it should only be called after Stop or if the processor was initially
  // constructed with 0 threads. Calling Start without calling Stop first will
  // have no effect.
  void Start(size_t num_worker_threads = 1);

  // Stops the worker threads. Blocks until the currently running jobs are
  // completed. Call Start to resume processing the queue.
  void Stop();

  // Queues an object and its processing function to be run on a worker thread.
  // Once completed, the object will be available to Dequeue().  Returns the
  // task ID.
  TaskId Enqueue(T obj, ProcessFn fn);

  // Queues an object and its processing function to be run on a worker thread.
  // Unlike Enqueue, once the processing is completed, the object will go out
  // of scope.  Returns the task ID.
  TaskId Execute(T obj, ProcessFn fn);

  // Dequeues a processed object by moving it to |out| and returns true.  If
  // there are no available objects, the function does not modify |out| and
  // returns false.
  bool Dequeue(T* out);

  // Attempts to cancel the task with |id|.  Returns false if |id| isn't valid,
  // is executing, or has already completed.
  bool Cancel(TaskId id);

 private:
  // Indicate what to do with an object once its been processed.
  enum CompletionFlag {
    kExecuteOnly,
    kAddToCompleteQueue,
  };

  // Internal data structure to represent the async request.
  struct Request {
    Request(TaskId id, T obj, ProcessFn fn, CompletionFlag completion_flag)
        : id(id),
          object(std::move(obj)),
          process(std::move(fn)),
          completion_flag(completion_flag) {}
    TaskId id;
    T object;
    ProcessFn process;
    CompletionFlag completion_flag;
  };
  using RequestPtr = std::unique_ptr<Request>;

  using Lock = std::unique_lock<std::mutex>;

  TaskId GetNextTaskId();

  void WorkerThread();

  ThreadSafeDeque<RequestPtr> process_queue_;
  ThreadSafeDeque<RequestPtr> complete_queue_;
  std::vector<std::thread> worker_threads_;

  std::mutex mutex_;
  TaskId next_task_id_ = 1;
};

template <typename T>
AsyncProcessor<T>::AsyncProcessor(size_t num_worker_threads) {
  Start(num_worker_threads);
}

template <typename T>
AsyncProcessor<T>::~AsyncProcessor() {
  // Drain the queue of any remaining requests.
  RequestPtr req = nullptr;
  while (process_queue_.PopFront(&req)) {
  }
  Stop();
}

template <typename T>
typename AsyncProcessor<T>::TaskId AsyncProcessor<T>::GetNextTaskId() {
  Lock lock(mutex_);
  const TaskId task_id = next_task_id_++;
  if (next_task_id_ == kInvalidTaskId) {
    ++next_task_id_;
  }
  return task_id;
}

template <typename T>
void AsyncProcessor<T>::Start(size_t num_worker_threads) {
  if (!worker_threads_.empty()) {
    return;
  }
  for (size_t i = 0; i < num_worker_threads; ++i) {
    worker_threads_.emplace_back([this]() { WorkerThread(); });
  }
}

template <typename T>
void AsyncProcessor<T>::Stop() {
  // An empty (nullptr) request signals the thread to finish.
  for (size_t i = 0; i < worker_threads_.size(); ++i) {
    process_queue_.PushFront(nullptr);
  }
  for (auto& thread : worker_threads_) {
    thread.join();
  }
  worker_threads_.clear();
}

template <typename T>
typename AsyncProcessor<T>::TaskId AsyncProcessor<T>::Execute(T obj,
                                                              ProcessFn fn) {
  const TaskId id = GetNextTaskId();
  RequestPtr req(new Request(id, std::move(obj), fn, kExecuteOnly));
  process_queue_.PushBack(std::move(req));
  return id;
}

template <typename T>
typename AsyncProcessor<T>::TaskId AsyncProcessor<T>::Enqueue(T obj,
                                                              ProcessFn fn) {
  const TaskId id = GetNextTaskId();
  RequestPtr req(new Request(id, std::move(obj), fn, kAddToCompleteQueue));
  process_queue_.PushBack(std::move(req));
  return id;
}

template <typename T>
bool AsyncProcessor<T>::Dequeue(T* out) {
  RequestPtr req = nullptr;
  if (complete_queue_.PopFront(&req)) {
    *out = std::move(req->object);
    return true;
  }
  return false;
}

template <typename T>
bool AsyncProcessor<T>::Cancel(TaskId id) {
  bool removed = false;
  process_queue_.RemoveIf([id, &removed](const RequestPtr& ptr) {
    if (ptr->id == id) {
      removed = true;
      return true;
    }
    return false;
  });
  return removed;
}

template <typename T>
void AsyncProcessor<T>::WorkerThread() {
  while (true) {
    RequestPtr req = process_queue_.WaitPopFront();
    if (req) {
      req->process(&req->object);
      if (req->completion_flag == kAddToCompleteQueue) {
        complete_queue_.PushBack(std::move(req));
      }
    } else {
      // A nullptr request signals the thread to finish.
      break;
    }
  }
}
}  // namespace lull

#endif  // LULLABY_UTIL_ASYNC_PROCESSOR_H_
