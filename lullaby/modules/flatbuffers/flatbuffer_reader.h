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

#ifndef LULLABY_MODULES_FLATBUFFERS_FLATBUFFER_READER_H_
#define LULLABY_MODULES_FLATBUFFERS_FLATBUFFER_READER_H_

#include <memory>
#include "flatbuffers/flatbuffers.h"
#include "lullaby/modules/flatbuffers/flatbuffer_native_types.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/optional.h"

namespace lull {

// Reads data from a flatbuffer into an instance of an object class generated
// by the Lullaby flatc code generator.
class FlatbufferReader {
 public:
  // Reads data from the flatbuffer::Table into the specified object.
  template <typename T>
  static void SerializeObject(T* obj, const flatbuffers::Table* table) {
    SerializeTable(obj, table);
  }

  // Reads a scalar value (eg. uint8, int32, float, double, etc.) from the
  // internal flatbuffer into |value|.
  template <typename T, typename U>
  void Scalar(T* value, uint16_t offset, U default_value) {
    if (vtable_) {
      // For a table, the scalar value can be obtained by calling GetField.
      *value = GetField<T>(offset, static_cast<T>(default_value));
    } else {
      // For structs, the scalar value is written directly at the specified
      // offset from the base address of the struct.
      *value = *GetStructField<T>(offset);
    }
  }

  // Reads a string from the internal flatbuffer into |value|.
  void String(std::string* value, uint16_t offset) {
    // Strings are represented as flatbuffers::String instances and can only be
    // stored in tables.  A pointer to the string representation is obtained
    // by calling GetPointer.
    const auto* rhs = GetPointer<flatbuffers::String>(offset);
    if (rhs) {
      *value = rhs->str();
    } else {
      value->clear();
    }
  }

  // Reads a struct of type T from the internal flatbuffer into |value|.
  template <typename T>
  void Struct(T* value, uint16_t offset) {
    if (vtable_) {
      // For tables, the base address for the struct can be obtained by calling
      // GetStruct.
      const uint8_t* obj = GetStruct<uint8_t>(offset);
      SerializeStruct(value, obj);
    } else {
      // For structs, the base address for the struct is at the given offset.
      const uint8_t* obj = GetStructField<uint8_t>(offset);
      SerializeStruct(value, obj);
    }
  }

  // Reads a struct of type T from the internal flatbuffer into |value| if
  // the data exists, otherwise it resets |value| to an empty state.
  template <typename T>
  void Struct(lull::Optional<T>* value, uint16_t offset) {
    const uint8_t* obj = GetStruct<uint8_t>(offset);
    if (obj) {
      value->emplace();
      Struct(value->get(), offset);
    } else {
      value->reset();
    }
  }

  // Reads a struct from the internal flatbuffer into |value|.
  template <typename T>
  void NativeStruct(T* value, uint16_t offset) {
    const size_t len = FlatbufferNativeType<T>::kFlatbufferStructSize;
    if (vtable_) {
      // For tables, the base address for the struct can be obtained by calling
      // GetStruct.
      const uint8_t* obj = GetStruct<uint8_t>(offset);
      *value = FlatbufferNativeType<T>::Read(obj, len);
    } else {
      // For structs, the base address for the struct is at the given offset.
      const uint8_t* obj = GetStructField<uint8_t>(offset);
      *value = FlatbufferNativeType<T>::Read(obj, len);
    }
  }

  // Reads a struct from the internal flatbuffer into |value| if the data
  // exists, otherwise it resets |value| to an empty state.
  template <typename T>
  void NativeStruct(lull::Optional<T>* value, uint16_t offset) {
    const uint8_t* obj = GetStruct<uint8_t>(offset);
    if (obj) {
      value->emplace();
      NativeStruct(value->get(), offset);
    } else {
      value->reset();
    }
  }

  // Reads a flatbuffer table from the internal flatbuffer.
  template <typename T>
  void Table(T* value, uint16_t offset) {
    const auto* table = GetPointer<flatbuffers::Table>(offset);
    SerializeTable(value, table);
  }

  // Reads a flatbuffer table from the internal flatbuffer if the data exists,
  // otherwise it resets |value| to an empty state.
  template <typename T>
  void Table(lull::Optional<T>* value, uint16_t offset) {
    const uint8_t* obj = GetStruct<uint8_t>(offset);
    if (obj) {
      value->emplace();
      Table(value->get(), offset);
    } else {
      value->reset();
    }
  }

  // Reads a flatbuffer table from the internal flatbuffer if the data exists,
  // otherwise it resets |value| to an empty state.
  template <typename T>
  void Table(std::shared_ptr<T>* value, uint16_t offset) {
    const uint8_t* obj = GetStruct<uint8_t>(offset);
    if (obj) {
      value->reset(new T());
      Table(value->get(), offset);
    } else {
      value->reset();
    }
  }

  // Serializes a flatbuffer union type.
  template <typename T, typename U>
  void Union(T* value, uint16_t offset, U default_type_value) {
    const auto* table = GetPointer<flatbuffers::Table>(offset);
    if (table) {
      const uint16_t type_offset =
          static_cast<uint16_t>(offset - sizeof(uint16_t));
      const U type_value = GetField<U>(type_offset, default_type_value);
      SerializeUnion(value, table, type_value);
    } else {
      value->reset();
    }
  }

  // Serializes an array of scalar values.
  template <typename T, typename U = T>
  void VectorOfScalars(std::vector<T>* value, uint16_t offset) {
    const auto* vec = GetPointer<flatbuffers::Vector<U>>(offset);
    if (vec) {
      const size_t num = vec->size();
      value->resize(num);
      for (unsigned int i = 0; i < num; ++i) {
        value->at(i) = vec->Get(i);
      }
    } else {
      value->clear();
    }
  }

  // Serializes an array of strings.
  void VectorOfStrings(std::vector<std::string>* value, uint16_t offset) {
    using StringRef = flatbuffers::Offset<flatbuffers::String>;
    using VectorType = flatbuffers::Vector<StringRef>;

    const auto* vec = GetPointer<const VectorType>(offset);
    if (vec) {
      const unsigned int num = vec->size();
      value->resize(num);
      for (unsigned int i = 0; i < num; ++i) {
        const flatbuffers::String* src = vec->Get(i);
        if (src) {
          value->at(i) = src->str();
        } else {
          value->at(i).clear();
        }
      }
    } else {
      value->clear();
    }
  }

  // Serializes an array of flatbuffer struct types.
  template <typename T>
  void VectorOfStructs(std::vector<T>* value, uint16_t offset) {
    using VectorType = flatbuffers::Vector<typename T::FlatBufferType>;

    const auto* src = GetPointer<VectorType>(offset);
    if (src) {
      const size_t num = src->size();
      value->resize(num);
      for (unsigned int i = 0; i < num; ++i) {
        const auto& obj = src->Get(i);
        SerializeStruct(&value->at(i), reinterpret_cast<const uint8_t*>(&obj));
      }
    } else {
      value->clear();
    }
  }

  // Serializes an array of flatbuffer struct types that have specified a
  // "native_type" attribute.
  template <typename T>
  void VectorOfNativeStructs(std::vector<T>* value, uint16_t offset) {
    using VectorType = flatbuffers::Vector<uint8_t>;

    const auto* src = GetPointer<VectorType>(offset);
    if (src) {
      const size_t num = src->size();
      value->resize(num);
      const uint8_t* data = src->Data();
      for (size_t i = 0; i < num; ++i) {
        const size_t len = FlatbufferNativeType<T>::kFlatbufferStructSize;
        value->at(i) = FlatbufferNativeType<T>::Read(data, len);
        data += len;
      }
    } else {
      value->clear();
    }
  }

  // Serializes an array of flatbuffer table types.
  template <typename T>
  void VectorOfTables(std::vector<T>* value, uint16_t offset) {
    using TableRef = flatbuffers::Offset<flatbuffers::Table>;
    using VectorType = flatbuffers::Vector<TableRef>;

    const auto* src = GetPointer<VectorType>(offset);
    if (src) {
      const size_t num = src->size();
      value->resize(num);
      for (unsigned int i = 0; i < num; ++i) {
        SerializeTable(&value->at(i), src->Get(i));
      }
    } else {
      value->clear();
    }
  }

  // Informs objects that this serializer will overwrite data.
  bool IsDestructive() const { return true; }

 private:
  explicit FlatbufferReader(const flatbuffers::Table* src) {
    data_ = reinterpret_cast<const uint8_t*>(src);
    // The first element in the table data points to the vtable.
    vtable_ = data_ - Read<int32_t>(data_);
    // The first element in the vtable is the size of the vtable.
    num_fields_ = Read<uint16_t>(vtable_);
  }

  explicit FlatbufferReader(const uint8_t* data) { data_ = data; }

  template <typename T>
  static void SerializeTable(T* dst, const flatbuffers::Table* src) {
    if (src && dst) {
      FlatbufferReader reader(src);
      dst->SerializeFlatbuffer(reader);
    }
  }

  template <typename T>
  static void SerializeStruct(T* dst, const uint8_t* src) {
    if (src && dst) {
      FlatbufferReader reader(src);
      dst->SerializeFlatbuffer(reader);
    }
  }

  template <typename T, typename U>
  static void SerializeUnion(T* dst, const flatbuffers::Table* src, U type) {
    const auto flatbuffer_type = static_cast<typename T::FlatBufferType>(type);
    if (src && dst) {
      FlatbufferReader reader(src);
      dst->SerializeFlatbuffer(flatbuffer_type, reader);
    }
  }

  template <typename T>
  static T Read(const uint8_t* ptr) {
    return *reinterpret_cast<const T*>(ptr);
  }

  uint16_t GetTableFieldOffset(uint16_t field_id) const {
    if (vtable_) {
      const uint8_t* index = vtable_ + field_id;
      return field_id < num_fields_ ? Read<uint16_t>(index) : 0;
    } else {
      return 0;
    }
  }

  // For structs, the given field is a direct offset into the byte buffer.
  template <typename T>
  const T* GetStructField(uint16_t offset) const {
    CHECK(vtable_ == nullptr);
    return reinterpret_cast<const T*>(data_ + offset);
  }

  template <typename T>
  T GetField(uint16_t field_id, T default_value) const {
    const uint16_t offset = GetTableFieldOffset(field_id);
    if (offset == 0) {
      return default_value;
    }
    return Read<T>(data_ + offset);
  }

  template <typename T>
  const T* GetPointer(uint16_t field_id) const {
    const uint16_t offset = GetTableFieldOffset(field_id);
    if (offset == 0) {
      return nullptr;
    }

    const uint8_t* ptr = data_ + offset;
    return reinterpret_cast<const T*>(ptr + Read<uint32_t>(ptr));
  }

  template <typename T>
  const T* GetStruct(uint16_t field_id) const {
    const uint16_t offset = GetTableFieldOffset(field_id);
    if (offset == 0) {
      return nullptr;
    }

    const uint8_t* ptr = data_ + offset;
    return reinterpret_cast<const T*>(ptr);
  }

  // The serializer is either pointing to a flatbuffer table or struct.  In
  // the case of tables, there are convenient accessor functions that can be
  // used to get values.  In the case of structs, all values are "inlined"
  // in a byte buffer and can be accessed directly via the offset.
  // The serializer is either pointing to a flatbuffer table or struct.  In
  // the case of tables, there are convenient accessor functions that can be
  // used to get values.  In the case of structs, all values are "inlined"
  // in a byte buffer and can be accessed directly via the offset.
  const uint8_t* data_ = nullptr;
  const uint8_t* vtable_ = nullptr;
  size_t num_fields_ = 0;
};

template <typename T>
inline void ReadFlatbuffer(T* obj, const flatbuffers::Table* table) {
  FlatbufferReader::SerializeObject(obj, table);
}

}  // namespace lull

#endif  // LULLABY_MODULES_FLATBUFFERS_FLATBUFFER_READER_H_
