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

#include "lullaby/modules/ecs/blueprint.h"

namespace lull {

Blueprint::Blueprint() : mode_(kWriteMode) {}

Blueprint::Blueprint(size_t buffer_size) : mode_(kWriteMode) {
  buffer_.emplace(buffer_size);
}

Blueprint::Blueprint(ArrayAccessorFn accessor, size_t count)
    : accessor_(std::move(accessor)), mode_(kReadMode), count_(count) {
  Prepare();
}

void Blueprint::FinishWriting() {
  mode_ = kReadMode;
  index_ = 0;
  Prepare();
}

void Blueprint::Next() {
  if (index_ < count_) {
    ++index_;
    Prepare();
  }
}

void Blueprint::Prepare() {
  if (index_ >= count_) {
    return;
  }

  if (buffer_) {
    PrepareFromBuffer();
  } else if (accessor_) {
    PrepareFromAccessor();
  }
}

void Blueprint::PrepareFromBuffer() {
  // Get the Entry for the object at the specified index_ from the table of
  // contents in the buffer.
  const void* ptr = buffer_->FrontAt(index_ * sizeof(Entry));
  const Entry* entry = reinterpret_cast<const Entry*>(ptr);

  // Update the "current" data to point to the object in the buffer.
  const void* data = buffer_->BackAt(entry->offset);
  current_.type = entry->type;
  current_.flatbuffer = flatbuffers::GetRoot<flatbuffers::Table>(data);
}

void Blueprint::PrepareFromAccessor() {
  const TypedFlatbuffer result = accessor_(index_);
  if (result.first != 0) {
    current_.type = BlueprintType::CreateFromSchemaNameHash(result.first);
    current_.flatbuffer = result.second;
  }
}

HashValue Blueprint::GetLegacyDefType() const {
  return current_.type.GetSchemaNameHash();
}

const flatbuffers::Table* Blueprint::GetLegacyDefData() const {
  if (current_.flatbuffer == nullptr && current_.native_object != nullptr) {
    WriteCurrentObjectToBuffer();
  }
  return current_.flatbuffer;
}

void Blueprint::WriteCurrentObjectToBuffer() const {
  if (!buffer_) {
    buffer_.emplace(kDefaultBufferSize);
  }

  // Unfortunately, serialization requires a non-const pointer, even though it
  // does not modify the object being serialized.
  void* ptr = const_cast<void*>(current_.native_object);

  // Write the object into the buffer.
  const size_t offset = current_.write_fn(ptr, buffer_.get());

  const void* data = buffer_->BackAt(offset);
  current_.flatbuffer = flatbuffers::GetRoot<const flatbuffers::Table>(data);
}

}  // namespace lull
