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

#ifndef REDUX_TOOLS_DEF_CODE_GENERATOR_DEF_DOCUMENT_H_
#define REDUX_TOOLS_DEF_CODE_GENERATOR_DEF_DOCUMENT_H_

#include <string>
#include <vector>

#include "redux/tools/def_code_generator/metadata_types.h"

namespace redux::tool {

// Stores the metadata for a "def" data definition file.
struct DefDocument {
  std::vector<std::string> includes;
  std::vector<EnumMetadata> enums;
  std::vector<StructMetadata> structs;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_DEF_CODE_GENERATOR_DEF_DOCUMENT_H_
