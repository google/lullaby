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

#include "lullaby/tools/common/json_utils.h"

extern "C" {
#include "libjsonnet.h"
}
#include "lullaby/util/logging.h"
#include "lullaby/util/string_view.h"
#include "lullaby/tools/common/file_utils.h"

namespace lull {
namespace tool {

namespace {
/// Callback used by jsonnet for opening files.
char* JsonnetImportCallback(void* ctx, const char* base, const char* rel,
                            char** found_here, int* success) {
  std::string data;
  *success = LoadFile(rel, false, &data) ? 1 : 0;
  if (*success == 0) {
    char* res = jsonnet_realloc((struct JsonnetVm*)ctx, nullptr, 1);
    res[0] = 0;
    return res;
  }

  // Found here may be returning a relative path.
  if (found_here) {
    // Set found_here to "." (root folder) so further imports will be done
    // correctly.
    *found_here = jsonnet_realloc((struct JsonnetVm*)ctx, nullptr, 2);
    (*found_here)[0] = '.';
    (*found_here)[1] = 0;
  }

  const size_t len = data.size();
  char* mem = jsonnet_realloc((struct JsonnetVm*)ctx, nullptr, len + 1);
  memcpy(mem, data.c_str(), len);
  mem[len] = 0;
  return mem;
}
}  // namespace

std::string ConvertJsonnetToJson(string_view jsonnet_filename) {
  std::string json;

  struct JsonnetVm* jvm = jsonnet_make();
  jsonnet_import_callback(jvm, JsonnetImportCallback, jvm);

  std::string data;
  if (!LoadFile(jsonnet_filename.c_str(), false, &data)) {
    LOG(ERROR) << "Could not load jsonnet file: " << jsonnet_filename.c_str();
    return json;
  }

  int error = 0;
  auto res = jsonnet_evaluate_snippet(jvm, jsonnet_filename.c_str(),
                                      data.c_str(), &error);
  if (error != 0 || res == nullptr) {
    LOG(ERROR) << "Jsonnet error [" << error << "]: " << (res ? res : "?");
    LOG(ERROR) << "Could not process jsonnet file: "
               << jsonnet_filename.c_str();
  } else {
    json = res;
  }
  if (res) {
    jsonnet_realloc(jvm, res, 0);
  }
  jsonnet_destroy(jvm);
  return json;
}

flatbuffers::DetachedBuffer JsonToFlatbuffer(string_view json_contents,
                                             string_view schema_file_path,
                                             string_view schema_type) {
  flatbuffers::Parser parser;

  std::string schema_file_contents;
  if (!LoadFile(schema_file_path.c_str(), false, &schema_file_contents)) {
    LOG(ERROR) << "Failed to load: " << schema_file_path.c_str();
    return flatbuffers::DetachedBuffer();
  }

  if (!parser.Parse(schema_file_contents.c_str())) {
    LOG(ERROR) << "Flatbuffer failed to parse schema: "
               << schema_file_path.c_str();
    return flatbuffers::DetachedBuffer();
  }

  if (!parser.SetRootType(schema_type.c_str())) {
    LOG(ERROR) << "Failed setting parser root type to " << schema_type.c_str();
    return flatbuffers::DetachedBuffer();
  }

  if (!parser.Parse(json_contents.c_str())) {
    LOG(ERROR) << "Flatbuffer parse from JSON failed. JSON contents: \n"
               << json_contents.c_str() << "\nError: " << parser.error_;
    return flatbuffers::DetachedBuffer();
  }
  return parser.builder_.ReleaseBufferPointer();
}

}  // namespace tool
}  // namespace lull
