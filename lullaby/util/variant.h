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

#ifndef LULLABY_BASE_VARIANT_H_
#define LULLABY_BASE_VARIANT_H_

#include <functional>
#include <unordered_map>
#include <vector>

#include "lullaby/util/common_types.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/type_util.h"
#include "lullaby/util/typeid.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

class Variant;
using VariantArray = std::vector<Variant>;
using VariantMap = std::unordered_map<HashValue, Variant>;

// Used to store an instance of any type that has a TypeId.
//
// This class is similar to C++17 std::any, but differs in the following ways:
//
// * Returns a Lullaby TypeId instead of the C++ RTTI type_info class.
// * Does not do any dynamic allocations.  It is effectively just the
//   "small-object optimization" (SOO) part of std::any.
// * Stored objects are limited to 64 bytes and 16-byte alignment.  This is
//   designed to store common types including std containers and mathfu types.
class Variant {
 private:
  // Helper for determining if U is actually a Variant.  This is used to ensure
  // the correct template specialization is used when assigning values to the
  // Variant.
  template <class U>
  using EnableIfNotVariant = typename std::enable_if<
      !std::is_base_of<Variant, typename std::decay<U>::type>::value>::type;

  // Helper for determining if U is a vector, but not a VariantArray.
  template <class U>
  using EnableIfNotVector = typename std::enable_if<
      !detail::IsVector<typename std::decay<U>::type>::kValue ||
      std::is_same<typename std::decay<U>::type, VariantArray>::value>::type;

  // Helper for determining if U is a map, but not a VariantMap.
  template <class U>
  using EnableIfNotMap = typename std::enable_if<
      !detail::IsMap<typename std::decay<U>::type>::kValue ||
      std::is_same<typename std::decay<U>::type, VariantMap>::value>::type;

  // Helper for determining if U is a optional
  template <class U>
  using EnableIfNotOptional = typename std::enable_if<
      !detail::IsOptional<typename std::decay<U>::type>::kValue>::type;

 public:
  // Default constructor, no value set.
  Variant() : type_(0), handler_(nullptr) {}

  // Copy constructor, copies variant value stored in |rhs|.
  Variant(const Variant& rhs) : type_(rhs.type_), handler_(rhs.handler_) {
    if (!rhs.Empty()) {
      rhs.handler_(kCopy, Data(), rhs.Data(), nullptr);
    }
  }

  // Move constructor, copies variant value stored in |rhs|.
  Variant(Variant&& rhs) : type_(rhs.type_), handler_(rhs.handler_) {
    if (!rhs.Empty()) {
      rhs.handler_(kMove, Data(), nullptr, rhs.Data());
      rhs.Clear();
    }
  }

  // Sets the variant value to |value| of type |T|.  Uses EnableIfNotVariant to
  // ensure that this constructor is not used when |T| is actually a |Variant|.
  template <typename T, typename U = EnableIfNotVariant<T>>
  Variant(T&& value) : Variant() {
    Set(std::forward<T>(value));
  }

  // Destructor, ensures any stored value is properly destroyed.
  ~Variant() { Clear(); }

  // Copies variant value from |rhs| if a value is set.
  Variant& operator=(const Variant& rhs) {
    if (this != &rhs) {
      Clear();
      if (!rhs.Empty()) {
        type_ = rhs.type_;
        handler_ = rhs.handler_;
        handler_(kCopy, Data(), rhs.Data(), nullptr);
      }
    }
    return *this;
  }

  // Moves variant value from |rhs| if a value is set.
  Variant& operator=(Variant&& rhs) {
    if (this != &rhs) {
      Clear();
      if (!rhs.Empty()) {
        type_ = rhs.type_;
        handler_ = rhs.handler_;
        handler_(kMove, Data(), nullptr, rhs.Data());
        rhs.Clear();
      }
    }
    return *this;
  }

  // Sets the variant to the specified |value|.
  template <typename T, typename U = EnableIfNotVariant<T>>
  Variant& operator=(T&& value) {
    Set(std::forward<T>(value));
    return *this;
  }

  // Returns true if no value is set, false otherwise.
  bool Empty() const { return type_ == 0 && handler_ == nullptr; }

  // Resets the Variant back to an unset state, destroying any stored value.
  void Clear() {
    if (!Empty()) {
      handler_(kDestroy, nullptr, nullptr, Data());
      type_ = 0;
      handler_ = nullptr;
    }
  }

  // Gets a pointer to the variant value if it is of type |T|, otherwise returns
  // nullptr.
  template <typename T>
  T* Get() {
    return GetImpl<T>(this);
  }

  // Gets a pointer to the variant value if it is of type |T|, otherwise returns
  // nullptr.
  template <typename T>
  const T* Get() const {
    return GetImpl<const T>(this);
  }

  // Gets the variant value if it is of type |T|, otherwise returns
  // |default_value|.
  template <typename T>
  const T& ValueOr(const T& default_value) const {
    const T* value = Get<T>();
    return value ? *value : default_value;
  }

  // Returns the TypeId of the value currently being stored.
  TypeId GetTypeId() const {
    return (handler_) ? handler_(kNone, nullptr, nullptr, nullptr) : 0;
  }

  // Serializes the variant using the given Archive.
  template <typename Archive>
  void Serialize(Archive archive) {
    // Supported serialization types:
    #define TYPE_SWITCH           \
        CASE(bool)                \
        CASE(int8_t)              \
        CASE(uint8_t)             \
        CASE(int16_t)             \
        CASE(uint16_t)            \
        CASE(int32_t)             \
        CASE(uint32_t)            \
        CASE(int64_t)             \
        CASE(uint64_t)            \
        CASE(float)               \
        CASE(double)              \
        CASE(std::string)         \
        CASE(mathfu::vec2)        \
        CASE(mathfu::vec2i)       \
        CASE(mathfu::vec3)        \
        CASE(mathfu::vec3i)       \
        CASE(mathfu::vec4)        \
        CASE(mathfu::vec4i)       \
        CASE(mathfu::quat)        \
        CASE(mathfu::mat4)        \
        CASE(mathfu::mat4)        \
        CASE(lull::VariantArray)  \
        CASE(lull::VariantMap)

    TypeId id = GetTypeId();
    archive(&id, Hash("type"));
    #define CASE(T)                           \
        if (id == lull::GetTypeId<T>()) {     \
          if (archive.IsDestructive()) {      \
            T t;                              \
            archive(&t, Hash("data"));        \
            operator=(std::move(t));          \
          } else {                            \
            archive(Get<T>(), Hash("data"));  \
          }                                   \
          return;                             \
        }
    TYPE_SWITCH;
    #undef CASE
    if (id == 0) {
      if (archive.IsDestructive()) {
        Clear();
      }
      return;
    }
    LOG(ERROR) << "Unsupported TypeId in Variant serialization: " << id;
    #undef TYPE_SWITCH
  }

  // Utility type to enable/disable the NumericCast function below.
  template <typename T>
  using EnableIfNumeric =
      typename std::enable_if<std::is_arithmetic<T>::value, T>;

  // Similar to Get(), but attempts to perform a static_cast on the underlying
  // type.  Both T and the stored type must be a numeric value (ie. int, float,
  // etc.).  If neither type is numeric, returns an empty value.
  template <typename T>
  auto NumericCast() const -> Optional<typename EnableIfNumeric<T>::type> {
    if (auto ptr = Get<int32_t>()) return static_cast<T>(*ptr);
    if (auto ptr = Get<float>()) return static_cast<T>(*ptr);
    if (auto ptr = Get<uint32_t>()) return static_cast<T>(*ptr);
    if (auto ptr = Get<int64_t>()) return static_cast<T>(*ptr);
    if (auto ptr = Get<uint64_t>()) return static_cast<T>(*ptr);
    if (auto ptr = Get<double>()) return static_cast<T>(*ptr);
    if (auto ptr = Get<int16_t>()) return static_cast<T>(*ptr);
    if (auto ptr = Get<uint16_t>()) return static_cast<T>(*ptr);
    if (auto ptr = Get<int8_t>()) return static_cast<T>(*ptr);
    if (auto ptr = Get<uint8_t>()) return static_cast<T>(*ptr);
    return NullOpt;
  }

 private:
  enum {
    kStoreSize = 128,  // Large enough to store a mathfu::mat4.
    kStoreAlign = 16,  // Aligned for mathfu types.
  };

  // Type of operations that may be performed on the variant value.
  enum Operation {
    kNone,
    kCopy,
    kMove,
    kDestroy,
  };

  using InternalTypeId = uintptr_t;
  using Buffer = std::aligned_storage<kStoreSize, kStoreAlign>::type;
  using HandlerFn = TypeId (*)(Operation, void*, const void*, void*);

  // Performs the specified operation (copy, move, etc.) on the provided
  // pointers.  Using a single function to handle all the various operations
  // that can be performed on the variant type reduces the size of this class.
  // Returns the lull::TypeId of the provided |Type|.
  // The parameters are dependent on the type of operation.  Specifically:
  //   kNone: all parameters are ignored.
  //   kCopy: Object of |Type| is copied from |from| pointer to |to| pointer
  //          using copy constructor.
  //   kMove: Object of |Type| is moved from |victim| pointer to |to| pointer
  //          using move constructor.
  //   kMove: Object of |Type| in |victim| is destroyed using destructor.
  template <typename Type>
  static TypeId HandlerImpl(Operation op, void* to, const void* from,
                            void* victim) {
    switch (op) {
      case kNone:
        break;
      case kCopy:
        new (to) Type(*static_cast<const Type*>(from));
        break;
      case kMove:
        new (to) Type(std::move(*static_cast<Type*>(victim)));
        break;
      case kDestroy:
        static_cast<Type*>(victim)->~Type();
        break;
    }
    return lull::GetTypeId<Type>();
  }

  // Returns a unique value for a given |Type|.
  template <typename Type>
  static InternalTypeId GetInternalTypeId() {
    // This static variable isn't actually used, only its address, so there are
    // no concurrency issues.
    static char dummy_var;
    return reinterpret_cast<InternalTypeId>(&dummy_var);
  }

  // Returns a pointer to the value stored in the |variant| if it is of type
  // |Ret|, otherwise returns nullptr.  This function is used to provide a
  // single implementation for both the const and non-cost versions of the
  // public Get() function.
  template <typename Ret, typename Self>
  static Ret* GetImpl(Self* variant) {
    using RetType = typename std::decay<Ret>::type;
    const InternalTypeId type = GetInternalTypeId<RetType>();
    if (type == variant->type_) {
      return static_cast<Ret*>(variant->Data());
    }
    return nullptr;
  }

  // Sets the variant to the specified |value|.
  template <typename T, typename U = EnableIfNotVariant<T>,
            typename V = EnableIfNotVector<T>, typename W = EnableIfNotMap<T>,
            typename X = EnableIfNotOptional<T>>
  void Set(T&& value) {
    SetImpl(std::forward<T>(value));
  }

  template <typename T>
  void SetImpl(T&& value) {
    static_assert(sizeof(value) <= kStoreSize, "Variant size too small.");
    static_assert(alignof(T) <= kStoreAlign, "Variant aligment too small.");
    using Type = typename std::decay<T>::type;

    Clear();

    new (Data()) Type(std::forward<T>(value));
    type_ = GetInternalTypeId<Type>();
    handler_ = &HandlerImpl<Type>;
  }

  // Sets the variant to the *value inside the Optional |value|
  template <typename T, typename U = EnableIfNotVariant<T>>
  void Set(const Optional<T>& value) {
    SetOptional(value);
  }
  template <typename T, typename U = EnableIfNotVariant<T>>
  void Set(Optional<T>&& value) {
    SetOptional(std::move(value));
  }

  template <typename T>
  void SetOptional(T&& value) {
    if (value) {
      SetImpl(std::move(*value));
    } else {
      Clear();
    }
  }

  // Sets the variant to the specified vector |value|, stored as a VariantArray.
  template <typename T, typename U = EnableIfNotVariant<T>>
  void Set(const std::vector<T>& value) {
    SetVector(value);
  }
  template <typename T, typename U = EnableIfNotVariant<T>>
  void Set(std::vector<T>&& value) {
    SetVector(std::move(value));
  }

  template <typename T>
  void SetVector(T&& value) {
    VariantArray out;
    out.reserve(value.size());
    for (auto& t : value) {
      out.emplace_back(std::move(t));
    }
    SetImpl(std::move(out));
  }

  // Sets the variant to the specified map |value|, stored as a VariantMap.
  template <typename T, typename U = EnableIfNotVariant<T>>
  void Set(const std::unordered_map<HashValue, T>& value) {
    SetMap(value);
  }
  template <typename T, typename U = EnableIfNotVariant<T>>
  void Set(std::unordered_map<HashValue, T>&& value) {
    SetMap(std::move(value));
  }
  template <typename T>
  void Set(const std::map<HashValue, T>& value) {
    SetMap(value);
  }
  template <typename T>
  void Set(std::map<HashValue, T>&& value) {
    SetMap(std::move(value));
  }

  template <typename T>
  void SetMap(T&& value) {
    VariantMap out;
    for (auto& kv : value) {
      out[kv.first] = std::move(kv.second);
    }
    SetImpl(std::move(out));
  }

  // Returns a pointer to the internal buffer storage.
  void* Data() { return &buffer_; }

  // Returns a pointer to the internal buffer storage.
  const void* Data() const { return &buffer_; }

  InternalTypeId type_;  // TypeId of the stored value, or 0 if no value stored.
  HandlerFn handler_;    // Function that will perform operations variant value.
  Buffer buffer_;        // Memory buffer to hold variant value.
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::Variant);
LULLABY_SETUP_TYPEID(lull::VariantArray);
LULLABY_SETUP_TYPEID(lull::VariantMap);

#endif  // LULLABY_BASE_VARIANT_H_
