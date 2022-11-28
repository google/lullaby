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

#ifndef REDUX_TOOLS_COMMON_JSON_UTILS_H_
#define REDUX_TOOLS_COMMON_JSON_UTILS_H_

#include <type_traits>

#include "redux/modules/base/hash.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/base/serialize_traits.h"
#include "magic_enum.hpp"
#include "rapidjson/document.h"
#include "rapidjson/error/error.h"

namespace redux::tool {

// Forward-declarations of functions (because json objects are recursive).
template <typename T>
void ReadJsonObject(T* obj, rapidjson::Value& jobj);
template <typename T>
void ReadJsonValue(T* value, rapidjson::Value& jobj);
template <typename T>
void ReadJsonArray(T* array, rapidjson::Value& jobj);

// Converts a Json value into a native C++ value.
template <typename T>
void ReadJsonValue(T* value, rapidjson::Value& jobj) {
  constexpr bool kIsEnum = std::is_enum_v<T>;
  constexpr bool kIsBoolean = std::is_same_v<T, bool>;
  constexpr bool kIsNumber = std::is_arithmetic_v<T>;
  constexpr bool kIsString = std::is_same_v<T, std::string>;
  constexpr bool kIsArray = IsVectorV<T>;

  if constexpr (kIsBoolean) {
    CHECK(jobj.IsBool());
    *value = jobj.GetBool();
  } else if constexpr (kIsEnum) {
    CHECK(jobj.IsString() || jobj.IsNumber());
    if (jobj.IsString()) {
      auto optional_value = magic_enum::enum_cast<T>(jobj.GetString());
      CHECK(optional_value.has_value());
      *value = optional_value.value();
    } else if (jobj.IsNumber()) {
      *value = static_cast<T>(jobj.GetInt());
    }
  } else if constexpr (kIsNumber) {
    CHECK(jobj.IsNumber());
    *value = static_cast<T>(jobj.GetDouble());
  } else if constexpr (kIsString) {
    CHECK(jobj.IsString());
    *value = jobj.GetString();
  } else if constexpr (kIsArray) {
    CHECK(jobj.IsArray());
    ReadJsonArray(value, jobj);
  } else {
    CHECK(jobj.IsObject());
    ReadJsonObject(value, jobj);
  }
}

// Converts a Json Array into the `array` which should be a std::vector of
// native C++ objects.s
template <typename T>
void ReadJsonArray(T* array, rapidjson::Value& jobj) {
  CHECK(jobj.IsArray());
  for (auto iter = jobj.Begin(); iter != jobj.End(); ++iter) {
    ReadJsonValue(&array->emplace_back(), *iter);
  }
}

// Converts a Json Object into a native C++ object using redux serialization.
template <typename T>
void ReadJsonObject(T* obj, rapidjson::Value& jobj) {
  CHECK(jobj.IsObject());
  auto fn = [&](auto& value, HashValue key) {
    for (auto iter = jobj.MemberBegin(); iter != jobj.MemberEnd(); ++iter) {
      if (Hash(iter->name.GetString()) == key) {
        ReadJsonValue(&value, iter->value);
      }
    }
  };
  obj->Serialize(fn);
}

// Converts a JSON string into a native C++ object.
template <typename T>
T ReadJson(const char* json) {
  rapidjson::Document doc;
  doc.Parse<rapidjson::kParseTrailingCommasFlag>(json);
  CHECK(doc.GetParseError() == rapidjson::kParseErrorNone);
  CHECK(doc.IsObject());

  T obj;
  ReadJsonObject(&obj, doc);
  return obj;
}

}  // namespace redux::tool

#endif  // REDUX_TOOLS_COMMON_JSON_UTILS_H_
