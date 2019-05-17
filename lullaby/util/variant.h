/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#include "lullaby/util/aligned_alloc.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/common_types.h"
#include "lullaby/util/entity.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/type_util.h"
#include "lullaby/util/typeid.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

class Variant;
using VariantArray = std::vector<Variant>;
using VariantMap = std::unordered_map<HashValue, Variant>;

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::Variant);
LULLABY_SETUP_TYPEID(lull::VariantArray);
LULLABY_SETUP_TYPEID(lull::VariantMap);

namespace lull {

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
  Variant() : type_(0), size_(0), handler_(nullptr) {}

  // Copy constructor, copies variant value stored in |rhs|.
  Variant(const Variant& rhs)
      : type_(rhs.type_), size_(0), handler_(rhs.handler_) {
    if (!rhs.Empty()) {
      Resize(rhs.size_);
      DoOp(kCopy, Data(), rhs.Data(), nullptr);
    }
  }

  // Move constructor, copies variant value stored in |rhs|.
  Variant(Variant&& rhs) : type_(0), size_(0), handler_(nullptr) {
    if (!rhs.Empty()) {
      Move(&rhs, this);
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
        Resize(rhs.size_);
        DoOp(kCopy, Data(), rhs.Data(), nullptr);
      }
    }
    return *this;
  }

  // Moves variant value from |rhs| if a value is set.
  Variant& operator=(Variant&& rhs) {
    if (this != &rhs) {
      Clear();
      if (!rhs.Empty()) {
        Move(&rhs, this);
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
  bool Empty() const { return type_ == 0; }

  // Resets the Variant back to an unset state, destroying any stored value.
  void Clear() {
    if (!Empty()) {
      DoOp(kDestroy, nullptr, nullptr, Data());
      FreeHeapData();
      type_ = 0;
      size_ = 0;
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
  TypeId GetTypeId() const { return type_; }

  // Serializes the variant using the given Archive.
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&type_, ConstHash("type"));
    if (type_ == 0) {
      if (archive.IsDestructive()) {
        Clear();
      }
      return;
    }

// Supported serialization types:
#define LULLABY_TYPE_SWITCH        \
  LULLABY_CASE(bool)               \
  LULLABY_CASE(int8_t)             \
  LULLABY_CASE(uint8_t)            \
  LULLABY_CASE(int16_t)            \
  LULLABY_CASE(uint16_t)           \
  LULLABY_CASE(int32_t)            \
  LULLABY_CASE(uint32_t)           \
  LULLABY_CASE(int64_t)            \
  LULLABY_CASE(uint64_t)           \
  LULLABY_CASE(float)              \
  LULLABY_CASE(double)             \
  LULLABY_CASE(std::string)        \
  LULLABY_CASE(mathfu::vec2)       \
  LULLABY_CASE(mathfu::vec2i)      \
  LULLABY_CASE(mathfu::vec3)       \
  LULLABY_CASE(mathfu::vec3i)      \
  LULLABY_CASE(mathfu::vec4)       \
  LULLABY_CASE(mathfu::vec4i)      \
  LULLABY_CASE(mathfu::quat)       \
  LULLABY_CASE(mathfu::rectf)      \
  LULLABY_CASE(mathfu::recti)      \
  LULLABY_CASE(mathfu::mat4)       \
  LULLABY_CASE(lull::Entity)       \
  LULLABY_CASE(lull::VariantArray) \
  LULLABY_CASE(lull::VariantMap)

#define LULLABY_CASE(T)                             \
  if (type_ == lull::GetTypeId<T>()) {      \
    if (archive.IsDestructive()) {          \
      T t;                                  \
      archive(&t, ConstHash("data"));       \
      operator=(std::move(t));              \
    } else {                                \
      archive(Get<T>(), ConstHash("data")); \
    }                                       \
    return;                                 \
  }
    LULLABY_TYPE_SWITCH;
#undef LULLABY_CASE
#undef LULLABY_TYPE_SWITCH

    bool is_enum = IsEnum();
    archive(&is_enum, ConstHash("is_enum"));
    if (is_enum) {
      size_ = sizeof(EnumType);
      handler_ = nullptr;
      EnumType* ptr = reinterpret_cast<EnumType*>(Data());
      archive(ptr, 0);
      return;
    }

    LOG(DFATAL) << "Unsupported TypeId in Variant serialization: " << type_;
    Clear();
  }

  // Similar to Get(), but will also attempt to cast similar types:
  //   static_cast if both T and the type are numeric (int, float, enum, etc.)
  //   Entity to/from certain numeric types. Empty() will turn into kNullEntity
  //   any of mathfu::rectf/i or mathfu::vec4/i into each other.
  //   Clock::duration from (u)int64_t (interpreted as nanoseconds)
  template <typename T>
  Optional<T> ImplicitCast() const {
    if (auto ptr = Get<T>()) {
      return *ptr;
    } else {
      return ImplicitCastImpl<T>();
    }
  }

 private:
  enum {
    // Embedded buffer size. Any types larger than this will be heap-allocated.
    // This is chosen to be just large enough to contain the common primitive
    // types (e.g. int, float, vector, string), so most variants don't require a
    // separate allocation.
    kStoreSize = 32,
    kStoreAlign = 16,  // Aligned for mathfu types.
  };

  // Enum types are assumed to be no larger than a uint64_t.
  using EnumType = uint64_t;
  static_assert(sizeof(EnumType) <= kStoreSize, "");

  // Type of operations that may be performed on the variant value.
  enum Operation {
    kCopy,
    kMove,
    kDestroy,
  };

  using Buffer = std::aligned_storage<kStoreSize, kStoreAlign>::type;
  using HandlerFn = void (*)(Operation, void*, const void*, void*);

  // Performs the specified operation (copy, move, etc.) on the provided
  // pointers.  Using a single function to handle all the various operations
  // that can be performed on the variant type reduces the size of this class.
  // The parameters are dependent on the type of operation.  Specifically:
  //   kCopy: Object of |Type| is copied from |from| pointer to |to| pointer
  //          using copy constructor.
  //   kMove: Object of |Type| is moved from |victim| pointer to |to| pointer
  //          using move constructor.
  //   kDestroy: Object of |Type| in |victim| is destroyed using destructor.
  template <typename Type>
  static void HandlerImpl(Operation op, void* to, const void* from,
                          void* victim) {
    switch (op) {
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
  }

  template <typename T>
  struct HasTypeId {
    static constexpr bool value =
        TypeIdTraits<typename std::decay<T>::type>::kHasTypeId;
  };

  // Returns a pointer to the value stored in the |variant| if it is of type
  // |Ret|, otherwise returns nullptr.  This function is used to provide a
  // single implementation for both the const and non-cost versions of the
  // public Get() function.
  template <typename Ret, typename Self>
  static auto GetImpl(Self* variant) ->
      typename std::enable_if<HasTypeId<Ret>::value, Ret*>::type {
    using RetType = typename std::decay<Ret>::type;
    const TypeId type = lull::GetTypeId<RetType>();
    if (type == variant->type_) {
      return static_cast<Ret*>(variant->Data());
    }
    return nullptr;
  }

  // Overload for when Ret does not have a TypeId. In this case, the type could
  // never be stored in a Variant, so we always return nullptr.
  template <typename Ret, typename Self>
  static auto GetImpl(Self* variant) ->
      typename std::enable_if<!HasTypeId<Ret>::value, Ret*>::type {
    return nullptr;
  }

  // Sets the variant to the specified |value|.
  template <typename T, typename U = EnableIfNotVariant<T>,
            typename V = EnableIfNotVector<T>, typename W = EnableIfNotMap<T>,
            typename X = EnableIfNotOptional<T>>
  void Set(T&& value) {
    SetImpl(std::forward<T>(value));
  }

  // Sets the variant to the specified |value| if it has a TypeId.
  template <typename T>
  auto SetImpl(T&& value) ->
      typename std::enable_if<HasTypeId<T>::value, void>::type {
    static_assert(alignof(T) <= kStoreAlign, "Variant aligment too small.");
    using Type = typename std::decay<T>::type;

    Clear();

    type_ = lull::GetTypeId<Type>();

    if (std::is_enum<Type>::value) {
      assert(sizeof(value) <= sizeof(EnumType));
      size_ = sizeof(EnumType);
      EnumType* const data = static_cast<EnumType*>(EmbeddedData());
      *data = 0;  // Fill excess bits to 0 so we don't serialize garbage.
      memcpy(data, &value, sizeof(value));
      handler_ = nullptr;
    } else {
      Resize(sizeof(value));
      new (Data()) Type(std::forward<T>(value));
      handler_ = &HandlerImpl<Type>;
    }
  }

  // Overload for SetImpl for when T does not have a TypeId.  In this case, do
  // nothing.
  template <typename T>
  auto SetImpl(T&& value) ->
      typename std::enable_if<!HasTypeId<T>::value, void>::type {
    Clear();
  }

  void DoOp(Operation op, void* to, const void* from, void* victim) {
    if (type_ == 0) {
      return;
    } else if (!IsEnum()) {
      handler_(op, to, from, victim);
    } else if (op == kCopy) {
      memcpy(to, from, sizeof(EnumType));
    } else if (op == kMove) {
      memcpy(to, victim, sizeof(EnumType));
    }
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

  // Sets the variant to the specified vector |value|, stored as a
  // VariantArray.
  template <typename T, typename U = EnableIfNotVariant<T>>
  void Set(const std::vector<T>& value) {
    SetVector(value);
  }
  template <typename T, typename U = EnableIfNotVariant<T>>
  void Set(std::vector<T>&& value) {
    SetVector(std::move(value));
  }
  void Set(ByteArray value) {
    SetImpl(std::move(value));
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

  // Get data for small types with embedded storage.
  void* EmbeddedData() {
    assert(size_ <= kStoreSize && size_ > 0);
    return &buffer_;
  }
  const void* EmbeddedData() const {
    return const_cast<Variant*>(this)->EmbeddedData();
  }

  // Get data for large types with heap storage.
  void* HeapData() {
    assert(size_ > kStoreSize);
    return *reinterpret_cast<void**>(&buffer_);
  }
  const void* HeapData() const {
    return const_cast<Variant*>(this)->HeapData();
  }

  // Get data irrespective of storage location.
  void* Data() {
    return size_ <= kStoreSize ? &buffer_ : *reinterpret_cast<void**>(&buffer_);
  }
  const void* Data() const {
    return const_cast<Variant*>(this)->Data();
  }

  // Set data to a heap-allocated pointer.
  void SetDataAsPointer(void* pointer) {
    *reinterpret_cast<void**>(&buffer_) = pointer;
  }

  // Free heap data if necessary.
  void FreeHeapData() {
    if (size_ > kStoreSize) {
      AlignedFree(HeapData());
    }
  }

  // Move storage between variants.
  static void Move(Variant* from, Variant* to) {
    assert(to->Empty());  // Destination should be cleared by caller.
    to->type_ = from->type_;
    to->size_ = from->size_;
    to->handler_ = from->handler_;

    if (!from->Empty()) {
      if (from->size_ <= kStoreSize) {
        to->DoOp(kMove, to->EmbeddedData(), nullptr, from->EmbeddedData());
        from->DoOp(kDestroy, nullptr, nullptr, from->Data());
      } else {
        // Heap data can be moved by copying the pointer.
        to->SetDataAsPointer(from->HeapData());
      }
    }

    from->type_ = 0;
    from->size_ = 0;
    from->handler_ = nullptr;
  }

  // Resize storage, freeing and reallocating heap memory as necessary.
  void Resize(size_t size) {
    if (size_ != size) {
      FreeHeapData();
      if (size > kStoreSize) {
        SetDataAsPointer(AlignedAlloc(size, kStoreAlign));
      }
      size_ = static_cast<uint32_t>(size);
    }
  }

  // Cast specialization for int and float casts.
  template <typename T>
  Optional<typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
  ImplicitCastImpl() const;

  // Cast specialization for enum types, only works on integers.
  template <typename T>
  Optional<typename std::enable_if<std::is_enum<T>::value, T>::type>
  ImplicitCastImpl() const;

  // Base template for other conversions, does nothing.
  template <typename T>
  Optional<typename std::enable_if<
      !std::is_enum<T>::value && !std::is_arithmetic<T>::value, T>::type>
  ImplicitCastImpl() const {
    return NullOpt;
  }

  // Note: This does not check Empty().
  bool IsEnum() const { return handler_ == nullptr; }

  TypeId type_ = 0;    // TypeId of the stored value, or 0 if no value stored.
  uint32_t size_;
  HandlerFn handler_;  // Used to perform type-specific operations on the value.
  Buffer buffer_;      // Memory buffer to hold variant value.
};

template <typename T>
Optional<typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
Variant::ImplicitCastImpl() const {
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
  if (auto ptr = Get<Entity>()) return static_cast<T>(ptr->AsUint32());
  if (!Empty() && IsEnum()) {
    const EnumType* ptr = reinterpret_cast<const EnumType*>(EmbeddedData());
    return static_cast<T>(*ptr);
  }
  return NullOpt;
}

template <typename T>
Optional<typename std::enable_if<std::is_enum<T>::value, T>::type>
Variant::ImplicitCastImpl() const {
  if (auto ptr = Get<int32_t>()) return static_cast<T>(*ptr);
  if (auto ptr = Get<uint32_t>()) return static_cast<T>(*ptr);
  if (auto ptr = Get<int64_t>()) return static_cast<T>(*ptr);
  if (auto ptr = Get<uint64_t>()) return static_cast<T>(*ptr);
  if (auto ptr = Get<int16_t>()) return static_cast<T>(*ptr);
  if (auto ptr = Get<uint16_t>()) return static_cast<T>(*ptr);
  if (auto ptr = Get<int8_t>()) return static_cast<T>(*ptr);
  if (auto ptr = Get<uint8_t>()) return static_cast<T>(*ptr);
  return NullOpt;
}

// Cast specialization for Entity from integers.
template <>
inline Optional<Entity> Variant::ImplicitCastImpl<Entity>() const {
  if (auto ptr = Get<uint32_t>()) return Entity(*ptr);
  if (auto ptr = Get<int64_t>()) return Entity(*ptr);
  if (auto ptr = Get<int32_t>()) return Entity(*ptr);
  if (auto ptr = Get<uint64_t>()) return Entity(*ptr);
  if (Empty()) return kNullEntity;
  return NullOpt;
}

// Cast specialization for vec4 from vec4i and rectf(i).
template <>
inline Optional<mathfu::vec4> Variant::ImplicitCastImpl<mathfu::vec4>() const {
  if (auto ptr = Get<mathfu::vec4i>()) {
    return mathfu::vec4(*ptr);
  }
  if (auto ptr = Get<mathfu::rectf>()) {
    return mathfu::vec4(ptr->pos, ptr->size);
  }
  if (auto ptr = Get<mathfu::recti>()) {
    return mathfu::vec4(mathfu::vec2(ptr->pos), mathfu::vec2(ptr->size));
  }
  return NullOpt;
}

// Cast specialization for vec4i from vec4 and rectf(i).
template <>
inline Optional<mathfu::vec4i> Variant::ImplicitCastImpl<mathfu::vec4i>()
    const {
  if (auto ptr = Get<mathfu::vec4>()) {
    return mathfu::vec4i(*ptr);
  }
  if (auto ptr = Get<mathfu::rectf>()) {
    return mathfu::vec4i(mathfu::vec2i(ptr->pos), mathfu::vec2i(ptr->size));
  }
  if (auto ptr = Get<mathfu::recti>()) {
    return mathfu::vec4i(ptr->pos, ptr->size);
  }
  return NullOpt;
}

// Cast specialization for rectf from vec4(i) and recti.
template <>
inline Optional<mathfu::rectf> Variant::ImplicitCastImpl<mathfu::rectf>()
    const {
  if (auto ptr = Get<mathfu::vec4>()) {
    return mathfu::rectf(*ptr);
  }
  if (auto ptr = Get<mathfu::vec4i>()) {
    return mathfu::rectf(mathfu::vec4(*ptr));
  }
  if (auto ptr = Get<mathfu::recti>()) {
    return mathfu::rectf(mathfu::vec2(ptr->pos), mathfu::vec2(ptr->size));
  }
  return NullOpt;
}

// Cast specialization for recti from vec4(i) and rectf.
template <>
inline Optional<mathfu::recti> Variant::ImplicitCastImpl<mathfu::recti>()
    const {
  if (auto ptr = Get<mathfu::vec4>()) {
    return mathfu::recti(mathfu::vec4i(*ptr));
  }
  if (auto ptr = Get<mathfu::vec4i>()) {
    return mathfu::recti(*ptr);
  }
  if (auto ptr = Get<mathfu::rectf>()) {
    return mathfu::recti(mathfu::vec2i(ptr->pos), mathfu::vec2i(ptr->size));
  }
  return NullOpt;
}

// Cast specialization for duration from (u)int64_t.
template <>
inline Optional<Clock::duration> Variant::ImplicitCastImpl<Clock::duration>()
    const {
  if (auto ptr = Get<int64_t>()) {
    return std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<int64_t, std::nano>(*ptr));
  }
  if (auto ptr = Get<uint64_t>()) {
    return std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<uint64_t, std::nano>(*ptr));
  }
  return NullOpt;
}

}  // namespace lull

#endif  // LULLABY_BASE_VARIANT_H_
