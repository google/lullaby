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

#include "lullaby/viewer/src/builders/build_blueprint.h"

#include "lullaby/viewer/entity_generated.h"
#include "lullaby/viewer/src/builders/flatbuffers.h"
#include "lullaby/viewer/src/builders/jsonnet.h"
#include "lullaby/viewer/src/file_manager.h"
#include "lullaby/util/filename.h"
#include "lullaby/tools/common/file_utils.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace lull {
namespace tool {

static bool IsValidComponentDef(const std::string& def_type) {
  for (int i = 0; i <= ComponentDefType_MAX; ++i) {
    ComponentDefType type = static_cast<ComponentDefType>(i);
    const char* valid_def = EnumNameComponentDefType(type);
    if (def_type == valid_def) {
      return true;
    }
  }
  return false;
}

bool BuildBlueprint(Registry* registry, const std::string& target,
                    const std::string& out_dir) {
  const std::string name = RemoveDirectoryAndExtensionFromFilename(target);

  if (GetExtensionFromFilename(target) != ".bin") {
    LOG(ERROR) << "Target file must be a .bin file: " << target;
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
  if (!file_manager->LoadFile("entity_schema.fbs", &schema)) {
    LOG(ERROR) << "Could not load entity_schema.fbs file";
    return false;
  }

  // Validate the json source, stripping any unsupported defs.
  rapidjson::Document json;
  json.Parse(src.c_str());
  if (!json.IsObject()) {
    LOG(ERROR) << "Could not parse json.";
    return false;
  } else if (!json.HasMember("components")) {
    LOG(ERROR) << "No components in json.";
    return false;
  }
  auto& components = json["components"];
  if (!components.IsArray()) {
    LOG(ERROR) << "Expected components array.";
    return false;
  }
  for (rapidjson::Value::ValueIterator iter = components.Begin();
       iter != components.End();) {
    if (iter->HasMember("def_type")) {
      const std::string def_type = (*iter)["def_type"].GetString();
      if (!IsValidComponentDef(def_type)) {
        iter = components.Erase(iter);
      } else {
        ++iter;
      }
    }
  }

  // Rewrite the "fixed" src to a string.
  rapidjson::StringBuffer json_buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(json_buffer);
  json.Accept(writer);
  src = json_buffer.GetString();

  // Generate the flatbuffer binary from the json src.
  auto buffer = ConvertJsonToFlatbuffer(src, schema);
  if (buffer.data() == nullptr) {
    LOG(ERROR) << "Error saving json to flatbuffer: " << target;
    return false;
  }

  // Save the binary file to the output folder.
  const std::string outfile = out_dir + name + ".bin";
  if (!SaveFile(buffer.data(), buffer.size(), outfile.c_str(), true)) {
    LOG(ERROR) << "Error saving file: " << name << ".bin";
    return false;
  }
  return true;
}

}  // namespace tool
}  // namespace lull
