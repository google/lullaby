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

#include "redux/tools/common/jsonnet_utils.h"

extern "C" {
#include "libjsonnet.h"
}
#include "redux/tools/common/file_utils.h"

namespace redux::tool {

// Callback used by jsonnet for opening files.
static char* JsonnetImportCallback(void* ctx, const char* base, const char* rel,
                                   char** found_here, int* success) {
  auto* jvm = static_cast<struct JsonnetVm*>(ctx);

  std::string data = LoadFileAsString(rel);
  *success = !data.empty();
  if (*success == 0) {
    char* res = jsonnet_realloc(jvm, nullptr, 1);
    res[0] = 0;
    return res;
  }

  if (found_here) {
    const size_t rel_len = strlen(rel);
    *found_here = jsonnet_realloc(jvm, nullptr, rel_len + 1);
    memcpy(*found_here, rel, rel_len);
    (*found_here)[rel_len] = 0;
  }

  const size_t len = data.size();
  char* mem = jsonnet_realloc(jvm, nullptr, len + 1);
  memcpy(mem, data.c_str(), len);
  mem[len] = 0;
  return mem;
}

std::string JsonnetToJson(const char* jsonnet, const char* filename,
                          const JsonnetVarMap& ext_vars) {
  struct JsonnetVm* jvm = jsonnet_make();
  jsonnet_import_callback(jvm, JsonnetImportCallback, jvm);
  for (const auto& kv : ext_vars) {
    jsonnet_ext_var(jvm, kv.first.c_str(), kv.second.c_str());
  }

  int error = 0;
  char* res = jsonnet_evaluate_snippet(jvm, filename, jsonnet, &error);
  if (error != 0 || res == nullptr) {
    if (res) {
      LOG(ERROR) << res;
    }
    LOG(FATAL) << "Jsonnet error (" << error << ") for: " << filename;
    return "";
  }

  std::string json = res;
  if (res) {
    jsonnet_realloc(jvm, res, 0);
  }
  jsonnet_destroy(jvm);
  return json;
}

}  // namespace redux::tool
