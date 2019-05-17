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

#ifndef LULLABY_TOOLS_COMPILE_BLUEPRINT_FROM_JSON_BLUEPRINT_FROM_JSON_COMPILER_H_
#define LULLABY_TOOLS_COMPILE_BLUEPRINT_FROM_JSON_BLUEPRINT_FROM_JSON_COMPILER_H_

#include <memory>
#include <string>
#include <vector>

#include "flatbuffers/idl.h"
#include "lullaby/modules/ecs/blueprint_builder.h"
#include "lullaby/util/span.h"
#include "lullaby/util/string_view.h"
#include "rapidjson/document.h"

namespace lull {
namespace tool {

// Compiles JSON representations of BlueprintDefs into flatbuffer binaries.
class BlueprintFromJsonCompiler {
 public:
  BlueprintFromJsonCompiler();

  // Parses the contents of |fbs_contents| to adds to the available schema.
  // |include_paths| is a null-terminated list of cstrings used to resolve any
  // include statements.
  // |fbs_filename| is the filename of the fbs.
  bool ParseFbs(string_view fbs_contents, const char** include_paths,
                string_view fbs_filename);

  // Compiles the contents of |json_contents| into a flatbuffer binary and
  // returns it, or an empty buffer if failed.
  flatbuffers::DetachedBuffer ParseJson(string_view json_contents);

 private:
  bool ParseJsonEntity(const rapidjson::Value& json_entity);
  std::string GetDefTypeName(string_view def_type);

  std::unique_ptr<flatbuffers::Parser> fb_parser_;
  detail::BlueprintBuilder blueprint_builder_;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_COMPILE_BLUEPRINT_FROM_JSON_BLUEPRINT_FROM_JSON_COMPILER_H_
