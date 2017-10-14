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

#include "lullaby/util/data_container.h"

#include "lullaby/util/logging.h"

namespace lull {

DataContainer::DataContainer() {}

DataContainer::DataContainer(DataPtr data, const size_t capacity,
                             const AccessFlags access)
    : DataContainer(std::move(data), 0, capacity, access) {}

DataContainer::DataContainer(DataPtr data, const size_t initial_size,
                             const size_t capacity, const AccessFlags access)
    : data_(std::move(data)),
      size_(initial_size),
      capacity_(capacity),
      access_(access) {
  if (size_ > capacity_) {
    LOG(DFATAL) << "Tried to create a DataContainer with initial size > "
                << "capacity! initial size = " << initial_size << ", "
                << "capacity = " << capacity;
    size_ = capacity_;
  }
}

const uint8_t* DataContainer::GetReadPtr() const {
  if (!IsReadable()) {
    LOG(ERROR) << "Tried to get read pointer without read access; "
               << "returning nullptr instead.";
    return nullptr;
  }

  return data_.get();
}

uint8_t* DataContainer::GetData() {
  if (!IsReadable() || !IsWritable()) {
    LOG(ERROR) << "Tried to get mutable pointer without read+write access; "
               << "returning nullptr instead.";
    return nullptr;
  }

  return data_.get();
}

uint8_t* DataContainer::GetAppendPtr(size_t size) {
  if (!IsWritable()) {
    LOG(ERROR) << "Tried to get append pointer without write access; "
               << "returning nullptr instead.";
    return nullptr;
  }

  if (GetSize() + size > GetCapacity()) {
    LOG(ERROR) << "Tried to get append pointer for size " << size
               << " but couldn't fit in container with current size "
               << GetSize() << " and max size " << GetCapacity()
               << ". Returning nullptr instead.";
    return nullptr;
  }

  // Get the append pointer from the current end of the data. Then bump
  // the size so the next time we grab the pointer, it'll be at the
  // end of the appended data.
  uint8_t* append_ptr = data_.get() + size_;
  size_ += size;
  return append_ptr;
}

bool DataContainer::Append(const uint8_t* data, size_t size) {
  uint8_t* append_ptr = GetAppendPtr(size);
  if (append_ptr == nullptr) {
    return false;
  }

  std::memcpy(append_ptr, data, size);
  return true;
}

DataContainer DataContainer::CreateHeapCopy() const {
  if (GetCapacity() == 0) {
    return DataContainer();
  }
  DataContainer copy = CreateHeapDataContainer(capacity_);
  if (!IsReadable()) {
    LOG(DFATAL) << "Can't copy unreadable data.";
  }
  if (!copy.Append(data_.get(), size_)) {
    LOG(DFATAL) << "Failed to copy data.";
  }
  return copy;
}

}  // namespace lull
