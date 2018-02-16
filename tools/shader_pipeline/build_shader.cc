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

#include "tools/shader_pipeline/build_shader.h"

#include "lullaby/util/logging.h"
#include "tools/common/json_utils.h"
#include "tools/shader_pipeline/process_shader_source.h"
#include "rapidjson/include/rapidjson/document.h"
#include "rapidjson/include/rapidjson/stringbuffer.h"
#include "rapidjson/include/rapidjson/writer.h"

namespace lull {
namespace tool {

namespace {
/// Helper function to process the code section of a json shader snippet.
void ProcessSnippetCodeSection(string_view field_name,
                               rapidjson::Value* snippet,
                               rapidjson::Document* document) {
  if (!snippet || !snippet->HasMember(field_name.c_str())) {
    return;
  }

  auto& json_string = (*snippet)[field_name.c_str()];
  std::string code_string = json_string.GetString();
  if (!ProcessShaderSource(&code_string)) {
    LOG(FATAL) << "Failed to process shader code snippet.";
  }
  json_string.SetString(code_string.c_str(), code_string.size(),
                        document->GetAllocator());
}

/// Helper function to add snippets from a json document to an existing snippets
/// array.
bool AddSnippetsFromJson(const std::string& json_string,
                         rapidjson::Value* snippets_array,
                         rapidjson::Document::AllocatorType* allocator) {
  if (!allocator) {
    LOG(FATAL) << "Missing allocator.";
    return false;
  }

  if (!snippets_array) {
    LOG(FATAL) << "Missing output snippets array.";
    return false;
  }

  rapidjson::Document json;
  json.Parse(json_string.c_str());
  if (!json.IsObject()) {
    LOG(FATAL) << "Could not parse json.";
    return false;
  }

  if (!json.HasMember("snippets")) {
    LOG(FATAL) << "No snippets in json.";
    return false;
  }

  auto& snippets = json["snippets"];
  if (!snippets.IsArray()) {
    LOG(FATAL) << "Expected snippets array.";
    return false;
  }

  for (rapidjson::Value::ValueIterator iter = snippets.Begin();
       iter != snippets.End(); ++iter) {
    rapidjson::Value* snippet = &*iter;
    ProcessSnippetCodeSection("code", snippet, &json);
    ProcessSnippetCodeSection("main_code", snippet, &json);

    rapidjson::Value copy(rapidjson::kObjectType);
    copy.CopyFrom(*snippet, *allocator);
    snippets_array->PushBack(copy.Move(), *allocator);
  }

  return true;
}

/// Helper function to create a shader stage from shader snippet files.
/// @param snippets Snippet file names to process.
/// @param stage_name Name for the shader stage to be created.
/// @param stages_array An array of stages to add the constructed stage to.
/// @param allocator Allocator for creating json objects (should be associated
/// with the stages_array).
bool CreateStageFromSnippetFiles(
    Span<string_view> snippets, string_view stage_name,
    rapidjson::Value* stages_array,
    rapidjson::Document::AllocatorType* allocator) {
  // Create a snippets array to store the snippets.
  rapidjson::Value snippets_jarray(rapidjson::kArrayType);

  // Process the jsonnet snippet files and convert them into a json string.
  for (const auto& file : snippets) {
    std::string jsonnet = tool::ConvertJsonnetToJson(file);
    if (jsonnet.empty()) {
      return false;
    }

    // Add the json snippet oobject into the snippets array.
    if (!AddSnippetsFromJson(jsonnet, &snippets_jarray, allocator)) {
      return false;
    }
  }

  // Set the snippet type of the snippet object.
  rapidjson::Value stage(rapidjson::kObjectType);
  stage.AddMember(
      "type",
      rapidjson::Value().SetString(stage_name.c_str(), stage_name.size()),
      *allocator);
  stage.AddMember("snippets", snippets_jarray.Move(), *allocator);

  // Add the stage to the output array object.
  stages_array->PushBack(stage, *allocator);

  return true;
}
}  // namespace

flatbuffers::DetachedBuffer BuildShader(const ShaderBuildParams& params) {
  static const char* kShaderSchemaType = "lull.ShaderDef";

  rapidjson::Document json;
  auto& object = json.SetObject();
  rapidjson::Value stages(rapidjson::kArrayType);

  // Process the different snippet types and add them to the stages object.
  CreateStageFromSnippetFiles(params.vertex_stages, "Vertex", &stages,
                              &json.GetAllocator());
  CreateStageFromSnippetFiles(params.fragment_stages, "Fragment", &stages,
                              &json.GetAllocator());

  // Add the stages object to the main object.
  object.AddMember("stages", stages.Move(), json.GetAllocator());

  // Write the jsonnet into a json.
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  json.Accept(writer);

  // Convert the json into a flatbuffer.
  return tool::JsonToFlatbuffer(
      buffer.GetString(), params.shader_schema_file_path, kShaderSchemaType);
}

}  // namespace tool
}  // namespace lull
