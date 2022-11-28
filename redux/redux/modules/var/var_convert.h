/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#ifndef REDUX_MODULES_VAR_VAR_CONVERT_H_
#define REDUX_MODULES_VAR_VAR_CONVERT_H_

#include <type_traits>
#include <vector>

#include "redux/modules/base/serialize_traits.h"
#include "redux/modules/base/typed_ptr.h"
#include "redux/modules/var/var.h"
#include "redux/modules/var/var_array.h"
#include "redux/modules/var/var_table.h"
#include "magic_enum.hpp"

namespace redux {

// This file provides two functions for converting between C++ types and Vars:
// FromVar and ToVar.
//
// The Var class itself can only be used to store types with redux TypeIds.
// Furthermore, the value stored in the Var can only be accessed as the same
// type.
//
// These two functions, FromVar and ToVar, support more complex use-cases. For
// example, FromVar will allow you to read a float value from a Var that is
// storing an int. Similarly, ToVar will set the value T from an
// std::optional<T> if it is set, or will set the Var to Empty if the optional
// is also empty.

// Attempts to read a value of type T, performing any necessary casts or
// converstions, from the Var, and assigns it to `out`. Returns true if
// successful.
template <typename T>
bool FromVar(const Var& var, T* out);

// Attempts to set the value, of type T, performing any necessary casts or
// converstions, to the Var. Returns true if successful.
template <typename T>
bool ToVar(const T& value, Var* out);

namespace detail {

// Performs a static_cast for arithmetic types, but uses magic_enum::enum_cast
// for enums.
template <typename From, typename To>
bool TryNumericCast(const From& value, To* out) {
  if constexpr (std::is_enum_v<To>) {
    using U = magic_enum::underlying_type_t<To>;
    auto result = magic_enum::enum_cast<To>(static_cast<U>(value));
    if (result.has_value()) {
      *out = result.value();
      return true;
    } else {
      return false;
    }
  } else {
    *out = static_cast<To>(value);
    return true;
  }
}

template <typename En, std::size_t... N>
absl::Span<const HashValue> EnumHashes(const std::index_sequence<N...>& seq) {
  static const HashValue hashes[] = {
    Hash(magic_enum::enum_name<magic_enum::enum_value<En>(N)>())...
  };
  CHECK_EQ(sizeof(hashes) / sizeof(hashes[0]), seq.size());
  return {hashes, seq.size()};
}

template <typename T>
bool DefaultFromVar(const Var& var, T* out) {
  if (var.Empty()) {
    *out = T{};
    return true;
  }

  const T* ptr = var.Get<T>();
  if (ptr) {
    *out = *ptr;
    return true;
  }

  if constexpr (std::is_same_v<T, std::string>) {
    if (auto x = var.Get<std::string_view>()) {
      *out = std::string(*x);
      return true;
    }
  }
  if constexpr (std::is_same_v<T, std::string_view>) {
    if (auto x = var.Get<std::string>()) {
      *out = std::string_view(*x);
      return true;
    }
  }
  if constexpr (std::is_same_v<T, TypeId>) {
    if (auto x = var.Get<HashValue>()) {
      *out = TypeId(x->get());
      return true;
    }
  }

  if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
#define REDUX_TRY_NUMERIC_CAST(U)         \
  if (auto x = var.Get<U>()) {            \
    return TryNumericCast<U, T>(*x, out); \
  }

    // The order is (loosely) based on a non-data-driven assumption on which
    // conversions will be more common.
    REDUX_TRY_NUMERIC_CAST(int);
    REDUX_TRY_NUMERIC_CAST(float);
    REDUX_TRY_NUMERIC_CAST(double);
    REDUX_TRY_NUMERIC_CAST(std::uint32_t);
    REDUX_TRY_NUMERIC_CAST(std::int64_t);
    REDUX_TRY_NUMERIC_CAST(std::uint64_t);
    REDUX_TRY_NUMERIC_CAST(std::int16_t);
    REDUX_TRY_NUMERIC_CAST(std::uint16_t);
    REDUX_TRY_NUMERIC_CAST(std::int8_t);
    REDUX_TRY_NUMERIC_CAST(std::uint8_t);
#undef REDUX_TRY_NUMERIC_CAST
  }

  if constexpr (std::is_arithmetic_v<T>) {
    if (auto x = var.Get<HashValue>()) {
      *out = static_cast<T>(x->get());
      return true;
    }
  }

  if constexpr (std::is_enum_v<T>) {
    if (auto x = var.Get<std::string_view>()) {
      auto result = magic_enum::enum_cast<T>(*x);
      if (result.has_value()) {
        *out = result.value();
        return true;
      }
    } else if (auto x = var.Get<std::string>()) {
      auto result = magic_enum::enum_cast<T>(*x);
      if (result.has_value()) {
        *out = result.value();
        return true;
      }
    } else if (auto x = var.Get<HashValue>()) {
      constexpr std::size_t kEnumCount = magic_enum::enum_count<T>();
      auto hashes = EnumHashes<T>(std::make_index_sequence<kEnumCount>());
      for (std::size_t i = 0; i < kEnumCount; ++i) {
        if (hashes[i] == *x) {
          *out = magic_enum::enum_value<T>(i);
          return true;
        }
      }
    }
  }

  return false;
}

template <typename T>
bool PointerFromVar(const Var& var, T** out) {
  if (var.GetTypeId() == GetTypeId<TypedPtr>()) {
    *out = var.ValueOr(TypedPtr()).Get<T>();
  } else {
    *out = const_cast<T*>(var.Get<T>());
  }
  return *out != nullptr;
}

template <typename T>
bool OptionalFromVar(const Var& var, std::optional<T>* out) {
  if (var.Is<T>()) {
    *out = *var.Get<T>();
    return true;
  } else {
    *out = std::nullopt;
    return var.Empty();
  }
}

template <typename T>
bool ArrayFromVar(const Var& var, std::vector<T>* out) {
  out->clear();

  if (const auto* array = var.Get<VarArray>()) {
    for (const auto& v : *array) {
      T t;
      if (!FromVar(v, &t)) {
        out->clear();
        return false;
      }
      out->emplace_back(std::move(t));
    }
    return true;
  } else if (!var.Empty()) {
    T t;
    if (!FromVar(var, &t)) {
      return false;
    }
    out->emplace_back(std::move(t));
    return true;
  } else {
    return var.Empty();
  }
}

template <typename K, typename V, typename H>
bool MapFromVar(const Var& var, absl::flat_hash_map<K, V, H>* out) {
  out->clear();

  if (const auto* table = var.Get<VarTable>()) {
    for (const auto& kv : *table) {
      K k;
      if (!FromVar(kv.first, &k)) {
        out->clear();
        return false;
      }
      V v;
      if (!FromVar(kv.second, &v)) {
        out->clear();
        return false;
      }
      out->emplace(k, std::move(v));
    }
    return true;
  } else {
    return var.Empty();
  }
}

template <typename T>
bool DefaultToVar(const T& value, Var* out) {
  *out = value;
  return true;
}

template <typename T>
bool PointerToVar(const T& value, Var* out) {
  if (value == nullptr) {
    *out = Var();
  } else {
    *out = TypedPtr(value);
  }
  return true;
}

template <typename T>
bool OptionalToVar(const std::optional<T>& value, Var* out) {
  if (value.has_value()) {
    return ToVar(value.value(), out);
  } else {
    *out = Var();
  }
  return true;
}

template <typename T>
bool ArrayToVar(const T& value, Var* out) {
  out->Clear();

  VarArray arr;
  for (const auto& v : value) {
    Var element;
    if (!ToVar(v, &element)) {
      return false;
    }
    arr.PushBack(std::move(element));
  }
  *out = std::move(arr);
  return true;
}

template <typename T>
bool MapToVar(const T& value, Var* out) {
  out->Clear();

  VarTable table;
  for (const auto& kv : value) {
    Var key;
    if (!ToVar(kv.first, &key)) {
      return false;
    }
    if (!key.Is<HashValue>()) {
      return false;
    }
    Var value;
    if (!ToVar(kv.second, &value)) {
      return false;
    }
    table.Insert(key.ValueOr(HashValue()), std::move(value));
  }
  *out = std::move(table);
  return true;
}
}  // namespace detail

template <typename T>
bool FromVar(const Var& var, T* out) {
  using U = typename std::decay<T>::type;
  if constexpr (IsPointerV<U>) {
    return detail::PointerFromVar(var, out);
  } else if constexpr (IsUniquePtrV<U>) {
    CHECK(false);
  } else if constexpr (IsOptionalV<U>) {
    return detail::OptionalFromVar(var, out);
  } else if constexpr (IsVectorV<U>) {
    return detail::ArrayFromVar(var, out);
  } else if constexpr (IsMapV<U>) {
    return detail::MapFromVar(var, out);
  } else {
    return detail::DefaultFromVar(var, out);
  }
}

template <typename T>
bool ToVar(const T& value, Var* out) {
  using U = typename std::decay<T>::type;
  if constexpr (IsPointerV<U>) {
    return detail::PointerToVar(value, out);
  } else if constexpr (IsUniquePtrV<U>) {
    return detail::PointerToVar(value.get(), out);
  } else if constexpr (IsOptionalV<U>) {
    return detail::OptionalToVar(value, out);
  } else if constexpr (IsVectorV<U>) {
    return detail::ArrayToVar(value, out);
  } else if constexpr (IsSpanV<U>) {
    return detail::ArrayToVar(value, out);
  } else if constexpr (IsMapV<U>) {
    return detail::MapToVar(value, out);
  } else {
    return detail::DefaultToVar(value, out);
  }
}
}  // namespace redux

#endif  // REDUX_MODULES_VAR_VAR_CONVERT_H_
