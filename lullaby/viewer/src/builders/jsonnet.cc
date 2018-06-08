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

#include "lullaby/viewer/src/builders/jsonnet.h"

extern "C" {
#include "libjsonnet.h"
}
#include "lullaby/util/logging.h"
#include "lullaby/tools/common/file_utils.h"
#include "lullaby/viewer/src/file_manager.h"

namespace lull {
namespace tool {

extern FileManager* g_file_manager;

// Callback used by jsonnet for opening files.
static char* JsonnetImportCallback(void* ctx, const char* base, const char* rel,
                                   char** found_here, int* success) {
  std::string data;
  *success = g_file_manager->LoadFile(rel, &data) ? 1 : 0;
  if (*success == 0) {
    char* res = jsonnet_realloc((struct JsonnetVm*)ctx, nullptr, 1);
    res[0] = 0;
    LOG(ERROR) << res;
    return res;
  }

  if (found_here) {
    *found_here = jsonnet_realloc((struct JsonnetVm*)ctx, nullptr, 2);
    (*found_here)[0] = '.';
    (*found_here)[1] = 0;
  }

  const size_t len = data.size();
  char* mem = jsonnet_realloc((struct JsonnetVm*)ctx, nullptr, len + 1);
  memcpy(mem, data.c_str(), len);
  mem[len] = 0;
  LOG(ERROR) << mem;
  return mem;
}

std::string ConvertJsonnetToJson(const std::string& jsonnet,
                                 const std::string& filename) {
  std::string json;

  struct JsonnetVm* jvm = jsonnet_make();
  jsonnet_import_callback(jvm, JsonnetImportCallback, jvm);

  int error = 0;
  auto res =
      jsonnet_evaluate_snippet(jvm, filename.c_str(), jsonnet.c_str(), &error);
  if (error != 0 || res == nullptr) {
    LOG(ERROR) << "Jsonnet error [" << error << "]: " << (res ? res : "?");
    LOG(ERROR) << "Could not process jsonnet file: " << filename;
  } else {
    json = res;
  }
  if (res) {
    jsonnet_realloc(jvm, res, 0);
  }
  jsonnet_destroy(jvm);
  return json;
}

}  // namespace tool
}  // namespace lull
