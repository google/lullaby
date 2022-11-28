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

#ifndef REDUX_TOOLS_DEF_CODE_GENERATOR_METADATA_TYPES_H_
#define REDUX_TOOLS_DEF_CODE_GENERATOR_METADATA_TYPES_H_

#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"

namespace redux::tool {

struct CommonMetadata {
  std::string name;
  std::string description;
};

struct TypeMetadata : CommonMetadata {
  std::string name_space;
};

struct EnumeratorMetadata : CommonMetadata {
  std::optional<uint64_t> value = 0;
};

struct EnumMetadata : TypeMetadata {
  std::string type = "uint64_t";
  std::vector<EnumeratorMetadata> enumerators;
};

struct FieldMetadata : CommonMetadata {
  std::string type;
  absl::flat_hash_map<std::string, std::string> attributes;

  // Some common attribute keys.
  static constexpr char kDefaulValue[] = "default_value";
};

struct StructMetadata : TypeMetadata {
  std::vector<FieldMetadata> fields;
  absl::flat_hash_map<std::string, std::string> attributes;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_DEF_CODE_GENERATOR_METADATA_TYPES_H_
