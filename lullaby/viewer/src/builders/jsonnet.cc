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

#include "lullaby/viewer/src/builders/jsonnet.h"

#include "lullaby/tools/common/jsonnet_utils.h"
#include "lullaby/viewer/src/file_manager.h"

namespace lull {
namespace tool {

extern FileManager* g_file_manager;

static bool DefaultJsonnetImporter(const char* filename, std::string* data) {
  return g_file_manager->LoadFile(filename, data);
}

std::string ConvertJsonnetToJson(const std::string& jsonnet,
                                 const std::string& filename) {
  return ConvertJsonnetToJson(jsonnet, DefaultJsonnetImporter, filename);
}

}  // namespace tool
}  // namespace lull
