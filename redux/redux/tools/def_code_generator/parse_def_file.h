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

#ifndef REDUX_TOOLS_DEF_CODE_GENERATOR_PARSE_DEF_FILE_H_
#define REDUX_TOOLS_DEF_CODE_GENERATOR_PARSE_DEF_FILE_H_

#include <string_view>

#include "absl/status/statusor.h"
#include "redux/tools/def_code_generator/def_document.h"

namespace redux::tool {

// Parses the given string into a DefDocument containing the metadata of the
// definitions described in the input.
//
// A def file can contain:
//
// - Include directive, in the format:
//     include "path/to/file/to/import.def"
//
// - Namespace directive, in the format:
//     namespace my.name.space
//
// - Enum defintion, in the format:
//     enum MyEnum {
//       Value1,
//       Value2,
//       Value3,
//     }
//
// - Struct definition, in the format:
//     struct MyStruct {
//       field_name: field_type = default_value
//     }
absl::StatusOr<DefDocument> ParseDefFile(std::string_view txt);

}  // namespace redux::tool

#endif  // REDUX_TOOLS_DEF_CODE_GENERATOR_PARSE_DEF_FILE_H_
