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

#ifndef LULLABY_MODULES_SERIALIZE_VARIANT_CONVERTER_H_
#define LULLABY_MODULES_SERIALIZE_VARIANT_CONVERTER_H_

#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "lullaby/modules/dispatcher/event_wrapper.h"
#include "lullaby/modules/serialize/serialize.h"
#include "lullaby/modules/serialize/serialize_traits.h"
#include "lullaby/modules/serialize/variant_serializer.h"
#include "lullaby/util/optional.h"
#include "lullaby/util/type_util.h"
#include "lullaby/util/variant.h"

namespace lull {

/// Functions that can be used to convert Variants to/from native values.
///
/// Conversions are only supported for the following types:
/// - C++ primitives: [u]int8, [u]int16, [u]int32, [u]int64, float, double.
/// - mathfu types: vec2, vec2i, vec3, vec3i, vec4, vec4i, mat4.
/// - std::string objects.
/// - EventWrapper objects.
/// - Objects with a Serialize function (see serialize.h).
/// - Optional<T> objects, where T is one of the supported types.
/// - std::vector<T> objects, where T is one of the supported types.
/// - std::unordered_map<HashValue, T> objects, where T is one of the supported
///   types.
class VariantConverter {
 public:
  /// Sets the value in |out| using data from the variant.  Returns true if
  /// successful, false otherwise.
  template <typename T>
  static bool FromVariant(const Variant& var, T* out) {
    return out ? ReadImpl(var, out) : false;
  }
  // Specialization for converting to a Variant.
  static bool FromVariant(const Variant& var, Variant* out) {
    *out = var;
    return true;
  }

  template <typename T>
  static bool ToVariant(const T& value, Variant* var) {
    return var ? WriteImpl(value, var) : false;
  }
  // Specialization for when T is a Variant.
  static bool ToVariant(const Variant& value, Variant* var) {
    *var = value;
    return true;
  }

 private:
  // Type trait that indicates T can be stored directly in a Variant.
  template <typename T>
  struct IsTrivial {
    // In addition to the "fundamental" types (eg. bool, int, float, mathfu),
    // we want to store strings and EventWrappers directly in Variants.
    static const bool kValue = detail::IsSerializeFundamental<T>::kValue ||
                               detail::IsString<T>::kValue ||
                               detail::IsEventWrapper<T>::kValue;
  };

  // Type trait that indicates T can be serialized to a Variant.  See
  // serialize.h and variant_serializers.h for more details.
  template <typename T>
  struct IsSerializable {
    // Optional has a Serialize function, but we want to handle Optional
    // more like a Variant, so we need to filter it out from here.
    static const bool kValue =
        detail::IsSerializable<T, LoadFromVariant>::kValue &&
        !detail::IsOptional<T>::kValue;
  };

  // Type trait that indiciates T is an Optional<>.
  template <typename T>
  using IsOptional = detail::IsOptional<T>;

  // Type trait that indiciates T is an std::vector<>.
  template <typename T>
  using IsVector = detail::IsVector<T>;

  // Type trait that indiciates T is an std::map<> or std::unordered_map<>.
  template <typename T>
  using IsMap = detail::IsMap<T>;

  // Saves "Trivial" type directly into Variant.
  template <typename T>
  static auto WriteImpl(const T& value, Variant* var) ->
      typename std::enable_if<IsTrivial<T>::kValue, bool>::type {
    *var = value;
    return true;
  }

  // Loads "Trivial" type directly out of Variant.
  template <typename T>
  static auto ReadImpl(const Variant& var, T* out) ->
      typename std::enable_if<IsTrivial<T>::kValue, bool>::type {
    const T* value = var.Get<T>();
    if (value == nullptr) {
      return false;
    }
    *out = *value;
    return true;
  }

  // Saves "Serializable" type to Variant using SaveToVariant serializer.
  template <typename T>
  static auto WriteImpl(const T& value, Variant* var) ->
      typename std::enable_if<IsSerializable<T>::kValue, bool>::type {
    VariantMap map;
    SaveToVariant save(&map);
    Serialize(&save, const_cast<T*>(&value), 0);
    *var = std::move(map);
    return true;
  }

  // Loads "Serializable" type from Variant using LoadFromVariant serializer.
  template <typename T>
  static auto ReadImpl(const Variant& var, T* out) ->
      typename std::enable_if<IsSerializable<T>::kValue, bool>::type {
    const VariantMap* value = var.Get<VariantMap>();
    if (value == nullptr) {
      return false;
    }
    LoadFromVariant load(value);
    Serialize(&load, out, 0);
    return true;
  }

  // Saves "Optional" type to Variant.  If the Optional stores a value, then
  // that value is copied into the Variant.  If the Optional is empty, then
  // the Variant is cleared.
  template <typename T>
  static auto WriteImpl(const T& value, Variant* var) ->
      typename std::enable_if<IsOptional<T>::kValue, bool>::type {
    if (value) {
      *var = value.value();
    } else {
      var->Clear();
    }
    return true;
  }

  // Loads "Optional" type from Variant.  If the Variant stores a value of the
  // same type as the Optional, then the value is copied into the Optional.
  // Otherwise, the Optional is cleared.
  template <typename T>
  static auto ReadImpl(const Variant& var, T* out) ->
      typename std::enable_if<IsOptional<T>::kValue, bool>::type {
    using ValueType = typename T::ValueType;

    if (var.Empty()) {
      out->reset();
      return true;
    }

    const auto* value = var.Get<ValueType>();
    if (value == nullptr) {
      return false;
    }
    *out = *value;
    return true;
  }

  // Saves "Vector" type into a VariantArray and then sets the Variant to the
  // VariantArray.
  template <typename T>
  static auto WriteImpl(const T& value, Variant* var) ->
      typename std::enable_if<IsVector<T>::kValue, bool>::type {
    VariantArray arr;
    for (const auto& elem : value) {
      Variant var;
      ToVariant(elem, &var);
      arr.emplace_back(std::move(var));
    }
    *var = std::move(arr);
    return true;
  }

  // Loads "Vector" type from a Variant by getting a VariantArray from the
  // Variant and then converting each element in the VariantArray into an
  // element to be stored in the vector.
  template <typename T>
  static auto ReadImpl(const Variant& var, T* out) ->
      typename std::enable_if<IsVector<T>::kValue, bool>::type {
    using ValueType = typename T::value_type;

    const VariantArray* src = var.Get<VariantArray>();
    if (src == nullptr) {
      return false;
    }

    T tmp;
    tmp.reserve(src->size());
    for (const auto& elem : *src) {
      ValueType value;
      if (!FromVariant(elem, &value)) {
        return false;
      }
      tmp.emplace_back(std::move(value));
    }
    *out = std::move(tmp);
    return true;
  }

  // Saves "Map" type into a VariantMap and then sets the Variant to the
  // VariantArray.
  template <typename T>
  static auto WriteImpl(const T& value, Variant* var) ->
      typename std::enable_if<IsMap<T>::kValue, bool>::type {
    VariantMap map;
    for (const auto& iter : value) {
      Variant var;
      ToVariant(iter.second, &var);
      map.emplace(iter.first, std::move(var));
    }
    *var = std::move(map);
    return true;
  }

  // Loads "Map" type from a Variant by getting a VariantMap from the Variant
  // and then converting each element in the VariantMap into a value to be
  // stored in the map with the same key.
  template <typename T>
  static auto ReadImpl(const Variant& var, T* out) ->
      typename std::enable_if<IsMap<T>::kValue, bool>::type {
    using KeyType = typename T::key_type;
    using ValueType = typename T::mapped_type;
    static_assert(std::is_same<KeyType, HashValue>::value,
                  "Keys must be HashValues");

    const VariantMap* src = var.Get<VariantMap>();
    if (src == nullptr) {
      return false;
    }

    T tmp;
    for (const auto& kv : *src) {
      ValueType value;
      if (!FromVariant(kv.second, &value)) {
        return false;
      }
      tmp.emplace(kv.first, std::move(value));
    }
    *out = std::move(tmp);
    return true;
  }
};

}  // namespace lull

#endif  // LULLABY_MODULES_SERIALIZE_VARIANT_CONVERTER_H_
