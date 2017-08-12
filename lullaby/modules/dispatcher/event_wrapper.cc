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

#include "lullaby/modules/dispatcher/event_wrapper.h"

namespace lull {

EventWrapper::EventWrapper(TypeId type, string_view name)
    : type_(type),
      size_(0),
      align_(0),
      ptr_(nullptr),
      data_(new VariantMap()),
      handler_(nullptr),
      owned_(kDoNotOwn),
      serializable_(true) {
#if LULLABY_TRACK_EVENT_NAMES
  name_ = name.to_string();
#endif
}

EventWrapper::EventWrapper(const EventWrapper& rhs)
    : type_(rhs.type_),
      size_(rhs.size_),
      align_(rhs.align_),
      ptr_(nullptr),
      data_(nullptr),
      handler_(rhs.handler_),
      owned_(kDoNotOwn),
      serializable_(rhs.serializable_) {
#if LULLABY_TRACK_EVENT_NAMES
  name_ = std::move(rhs.name_);
#endif
  if (rhs.ptr_) {
    ptr_ = AlignedAlloc(size_, align_);
    owned_ = kTakeOwnership;
    handler_(kCopy, ptr_, rhs.ptr_);
  }
  if (rhs.data_) {
    data_.reset(new VariantMap(*rhs.data_));
  }
}

EventWrapper::EventWrapper(EventWrapper&& rhs)
    : type_(rhs.type_),
      size_(rhs.size_),
      align_(rhs.align_),
      ptr_(nullptr),
      data_(nullptr),
      handler_(nullptr),
      owned_(kDoNotOwn),
      serializable_(rhs.serializable_) {
#if LULLABY_TRACK_EVENT_NAMES
  name_ = rhs.name_;
#endif
  operator=(std::move(rhs));
}

EventWrapper& EventWrapper::operator=(const EventWrapper& rhs) {
  return operator=(EventWrapper(rhs));
}

EventWrapper& EventWrapper::operator=(EventWrapper&& rhs) {
  using std::swap;
  swap(type_, rhs.type_);
  swap(size_, rhs.size_);
  swap(align_, rhs.align_);
  swap(ptr_, rhs.ptr_);
  swap(data_, rhs.data_);
  swap(handler_, rhs.handler_);
  swap(owned_, rhs.owned_);
  swap(serializable_, rhs.serializable_);
#if LULLABY_TRACK_EVENT_NAMES
  swap(name_, rhs.name_);
#endif
  return *this;
}

EventWrapper::~EventWrapper() {
  if (owned_ == kTakeOwnership) {
    handler_(kDestroy, ptr_, nullptr);
    AlignedFree(ptr_);
  }
}

void EventWrapper::SetValues(const VariantMap& values) {
  if (ptr_) {
    LOG(ERROR) << "Cannot set value on a concrete event.";
    return;
  }

  if (data_) {
    *data_ = values;
  }
}

const VariantMap* EventWrapper::GetValues() const {
  EnsureRuntimeEventAvailable();
  return data_ ? data_.get() : nullptr;
}

}  // namespace lull
