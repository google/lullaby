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

#ifndef LULLABY_MODULES_ECS_BLUEPRINT_H_
#define LULLABY_MODULES_ECS_BLUEPRINT_H_

#include <memory>
#include "lullaby/modules/ecs/blueprint_type.h"
#include "lullaby/modules/flatbuffers/flatbuffer_reader.h"
#include "lullaby/modules/flatbuffers/flatbuffer_writer.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/optional.h"
#include "lullaby/util/span.h"

namespace lull {

// Blueprints are used by Systems to read and write Component state.
//
// Systems define schemas (ie. fbs files) which, in turn, define (ie. generate)
// C++ classes. During a "create" operation, Systems use instances of the
// schema-defined C++ object to initialize Component data.  Similarly, during
// "save" operations, Systems extract data from a Component into an instance of
// a schema-defined C++ object.
//
// Blueprints provide a way for Systems to read/write the schema-defined C++
// objects using the Is<T>(), Read<T>(), and Write<T>() functions.  Furthermore,
// Blueprints hide the underlying format (eg. runtime or serialized) of these
// objects from Systems.
//
// Internally, Blueprints are either in Read or Write mode.  Blueprints are
// created to hold data (ie. data is written to the Blueprint) after which they
// are used to access the data (ie. data is read from the Blueprint).  Once read
// operations begin on a Blueprint, no further write operations are allowed.
//
// Though Blueprints "store" multiple objects, they only operate on a single
// object at a time.  In other words, Blueprints do not provide random access
// to arbitrary objects.  Instead, a ForEach function is provided that allows
// callers to Read the objects stored in the Blueprints in sequence.
//
// For backwards-compatibility reasons, Blueprints also provide a way to read
// the state data as a "HashValue + flatbuffers::Table*".  These functions have
// the "GetLegacy" prefix.
class Blueprint {
 public:
  // Associates a name with a flatbuffer table so that the table can be cast
  // to the correct type.
  using TypedFlatbuffer = std::pair<HashValue, const flatbuffers::Table*>;

  // Creates an empty Blueprint.  Objects should first be added to the Blueprint
  // (by calling Write) after which they can be read (by calling ForEach/Read).
  Blueprint();

  // Creates an empty Blueprint (like the default constructor), but with an
  // explicitly sized internal buffer.
  explicit Blueprint(size_t buffer_size);

  // Creates a Blueprint that wraps a single object.  This Blueprint can only be
  // used for reading (by calling ForEach/Read).
  template <typename T>
  explicit Blueprint(const T* ptr);

  // Creates a blueprint that will extract flatbuffers from an array.  It uses
  // an std::function to perform type-erasure of the array so that the Blueprint
  // does not need to know how the actual array is managed.
  using ArrayAccessorFn = std::function<TypedFlatbuffer(size_t index)>;
  Blueprint(ArrayAccessorFn accessor_fn, size_t count);

  // Returns the current type of the data in the blueprint (for reading).
  template <typename T>
  bool Is() const;

  // Reads data from the Blueprint into the provided object.
  template <typename T>
  bool Read(T* ptr) const;

  // Writes data from the provided object into the Blueprint.
  template <typename T>
  void Write(T* ptr);

  // Switches the Blueprint from write mode to read mode.
  void FinishWriting();

  // Allows the function |fn| to take the contents of the Blueprint and build a
  // final flatbuffer containing them.  Returns a Span pointing to the data
  // inside the buffer.
  template <typename Fn>
  Span<uint8_t> Finalize(const Fn& fn);

  // Iterates through all the objects currently stored in the blueprint for
  // reading.  The signature for Fn should be: void(const Blueprint&).
  template <typename Fn>
  void ForEachComponent(const Fn& fn);

  // These functions are provided for legacy Systems as they operate directly
  // on flatbuffers::Table+HashValue defs.  If the Blueprint is storing a raw
  // pointer to a native object internally, this function will serialize the
  // object into a flatbuffer.
  const flatbuffers::Table* GetLegacyDefData() const;
  HashValue GetLegacyDefType() const;

 private:
  enum { kDefaultBufferSize = 256 };

  using WriteToBufferFn = size_t (*)(void* ptr, InwardBuffer* buffer);

  enum Mode {
    kReadMode,
    kWriteMode,
  };

  // Represents an entry in the table of contents for the objects serialized
  // into the buffer.
  struct Entry {
    BlueprintType type;
    size_t offset;
  };

  // Represents a single object instance in the Blueprint.  The object will
  // either be stored as a raw_pointer (native_object) or as a flatbuffer table.
  struct TypedBlueprintData {
    BlueprintType type;
    const void* native_object = nullptr;
    const flatbuffers::Table* flatbuffer = nullptr;
    WriteToBufferFn write_fn = nullptr;
  };

  template <typename T>
  static size_t WriteToBuffer(void* ptr, InwardBuffer* buffer);

  void Prepare();
  void Next();
  void PrepareFromBuffer();
  void PrepareFromAccessor();
  void WriteCurrentObjectToBuffer() const;

  // Buffer used to serialize objects into flatbuffer binaries.
  mutable lull::Optional<InwardBuffer> buffer_;
  mutable TypedBlueprintData current_;  // The "current" data to be read.
  ArrayAccessorFn accessor_;  // The function used to extract flatbuffers.
  Mode mode_ = kReadMode;  // The current mode of operation.
  size_t index_ = 0;  // The index of the current data.
  size_t count_ = 0;  // The total number of objects stored in the Blueprint.
};

template <typename T>
Blueprint::Blueprint(const T* ptr) : mode_(kReadMode), index_(0), count_(1) {
  current_.type = BlueprintType::Create<T>();
  current_.native_object = reinterpret_cast<const void*>(ptr);
  current_.write_fn = WriteToBuffer<T>;
  Prepare();
}

template <typename T>
void Blueprint::Write(T* ptr) {
  if (mode_ != kWriteMode) {
    LOG(DFATAL) << "Must be in kWriteMode to write.";
    return;
  }
  if (!buffer_) {
    buffer_.emplace(kDefaultBufferSize);
  }

  WriteToBuffer<T>(ptr, buffer_.get());
  ++count_;
}

template <typename T>
size_t Blueprint::WriteToBuffer(void* ptr, InwardBuffer* buffer) {
  size_t offset = 0;
  if (buffer) {
    WriteFlatbuffer(reinterpret_cast<T*>(ptr), buffer);
    offset = buffer->BackSize();
  }

  Entry entry;
  entry.type = BlueprintType::Create<T>();
  entry.offset = offset;
  buffer->WriteFront(entry);
  return offset;
}

template <typename T>
bool Blueprint::Is() const {
  return current_.type.Is<T>();
}

template <typename T>
bool Blueprint::Read(T* ptr) const {
  if (mode_ != kReadMode) {
    LOG(DFATAL) << "Must be in ReadMode to read.";
    return false;
  }
  if (!current_.type.Is<T>()) {
    LOG(DFATAL) << "Invalid type.";
    return false;
  }

  if (current_.native_object) {
    *ptr = *(reinterpret_cast<const T*>(current_.native_object));
    return true;
  } else if (current_.flatbuffer) {
    FlatbufferReader::SerializeObject(ptr, current_.flatbuffer);
    return true;
  } else {
    LOG(DFATAL) << "No data to read from.";
    return false;
  }
}

template <typename Fn>
Span<uint8_t> Blueprint::Finalize(const Fn& fn) {
  if (count_ == 0) {
    return Span<>();
  }

  if (!buffer_) {
    WriteCurrentObjectToBuffer();
  }
  FinishWriting();

  FlatbufferWriter writer(buffer_.get());
  fn(&writer, this);

  const size_t size = buffer_->BackSize();
  const uint8_t* data = static_cast<const uint8_t*>(buffer_->BackAt(size));
  return Span<>(data, size);
}

template <typename Fn>
void Blueprint::ForEachComponent(const Fn& fn) {
  FinishWriting();
  for (size_t i = 0; i < count_; ++i) {
    fn(*this);
    Next();
  }
}

}  // namespace lull

#endif  // LULLABY_MODULES_ECS_BLUEPRINT_H_
