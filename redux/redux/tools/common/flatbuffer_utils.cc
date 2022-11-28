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

#include "redux/tools/common/flatbuffer_utils.h"

#include "redux/tools/common/file_utils.h"

namespace redux::tool {

static bool FlatbuffersLoadFileCallback(const char* filename, bool binary,
                                        std::string* dest) {
  auto data = LoadFile(filename);
  if (data.ok() && data->GetNumBytes() > 0) {
    *dest = std::string(reinterpret_cast<const char*>(data->GetBytes()),
                        data->GetNumBytes());
    return true;
  }
  return false;
}

flatbuffers::DetachedBuffer JsonToFlatbuffer(const char* json,
                                             const char* schema_file,
                                             const char* schema_name) {
  flatbuffers::SetLoadFileFunction(FlatbuffersLoadFileCallback);

  const std::string schema = LoadFileAsString(schema_file);

  flatbuffers::Parser parser;
  CHECK(parser.Parse(schema.c_str()))
      << "Flatbuffer failed to parse schema: " << schema_file;
  
  CHECK(parser.SetRootType(schema_name))
      << "Failed setting parser root type to: " << schema_name;

  CHECK(parser.Parse(json)) << "Failed to parse from JSON: " << parser.error_;

  flatbuffers::SetLoadFileFunction(nullptr);
  return parser.builder_.Release();
}

}  // namespace redux::tool
