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

#include "lullaby/viewer/src/builders/build_stategraph.h"

#include "lullaby/util/filename.h"
#include "lullaby/tools/common/file_utils.h"
#include "lullaby/viewer/src/builders/flatbuffers.h"
#include "lullaby/viewer/src/builders/jsonnet.h"
#include "lullaby/viewer/src/file_manager.h"

namespace lull {
namespace tool {

bool BuildStategraph(Registry* registry, const std::string& target,
                     const std::string& out_dir) {
  const std::string name = RemoveDirectoryAndExtensionFromFilename(target);

  if (GetExtensionFromFilename(target) != ".stategraph") {
    LOG(ERROR) << "Target file must be a .stategraph file: " << target;
    return false;
  }

  auto* file_manager = registry->Get<FileManager>();

  std::string src;
  if (!file_manager->LoadFile(name + ".jsonnet", &src)) {
    if (!file_manager->LoadFile(name + ".json", &src)) {
      LOG(ERROR) << "Count not find json/jsonnet file for: " << target;
    }
  }

  src = ConvertJsonnetToJson(src, name + ".jsonnet");
  if (src.empty()) {
    LOG(ERROR) << "Error loading/processing jsonnet source file: " << target;
    return false;
  }

  std::string schema;
  if (!file_manager->LoadFile("animation_stategraph.fbs", &schema)) {
    LOG(ERROR) << "Could not load entity_schema.fbs file";
    return false;
  }

  // Generate the flatbuffer binary from the json src.
  auto buffer = ConvertJsonToFlatbuffer(src, schema);
  if (buffer.data() == nullptr) {
    LOG(ERROR) << "Error saving json to flatbuffer: " << target;
    return false;
  }

  // Save the binary file to the output folder.
  const std::string outfile = out_dir + name + ".stategraph";
  if (!SaveFile(buffer.data(), buffer.size(), outfile.c_str(), true)) {
    LOG(ERROR) << "Error saving file: " << name << ".stategraph";
    return false;
  }
  return true;
}

}  // namespace tool
}  // namespace lull
