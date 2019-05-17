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

#include "lullaby/tools/shader_pipeline/build_shader.h"

#include <algorithm>

#include "lullaby/util/hash.h"
#include "lullaby/util/logging.h"
#include "lullaby/tools/common/json_utils.h"
#include "lullaby/tools/shader_pipeline/process_shader_source.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace lull {
namespace tool {

namespace {
static constexpr int kUnspecifiedVersion = 0;
static constexpr HashValue kAttributeHashPosition = ConstHash("ATTR_POSITION");
static constexpr HashValue kAttributeHashUV = ConstHash("ATTR_UV");
static constexpr HashValue kAttributeHashColor = ConstHash("ATTR_COLOR");
static constexpr HashValue kAttributeHashNormal = ConstHash("ATTR_NORMAL");
static constexpr HashValue kAttributeHashOrientation =
    ConstHash("ATTR_ORIENTATION");
static constexpr HashValue kAttributeHashTangent = ConstHash("ATTR_TANGENT");
static constexpr HashValue kAttributeHashBoneIndices =
    ConstHash("ATTR_BONE_INDICES");
static constexpr HashValue kAttributeHashBoneWeights =
    ConstHash("ATTR_BONE_WEIGHTS");

// Helper function to add a value to a json array only if that value isn't
// already present.
template <typename ValueType>
void AddUnique(ValueType value, rapidjson::Value* array,
               rapidjson::Document* document) {
  if (!array || !document) {
    return;
  }

  for (const auto& it : array->GetArray()) {
    if (it == value) {
      return;
    }
  }
  array->PushBack(value, document->GetAllocator());
}

// Helper function that ensures there is a version in the snippet.
void CheckForVersion(rapidjson::Value* snippet, rapidjson::Document* document) {
  if (!snippet || !document) {
    return;
  }

  if (!snippet->HasMember("versions")) {
    rapidjson::Value versions(rapidjson::kArrayType);
    rapidjson::Value version(rapidjson::kObjectType);
    version.AddMember("lang", "GL_Compat", document->GetAllocator());
    versions.PushBack(version, document->GetAllocator());
    snippet->AddMember("versions", versions, document->GetAllocator());
  }
}

// Helper function to create the environment flags of a snippet.
void CreateSnippetEnvironmentFlags(rapidjson::Value* snippet,
                                   rapidjson::Document* document) {
  if (!snippet || !document) {
    return;
  }

  if (!snippet->HasMember("environment")) {
    rapidjson::Value environment_flags(rapidjson::kArrayType);
    snippet->AddMember("environment", environment_flags,
                       document->GetAllocator());
  }
  rapidjson::Value& environment_flags = (*snippet)["environment"];

  // Iterate over the samplers to add environment flags.
  if (snippet->HasMember("samplers")) {
    const auto& samplers = (*snippet)["samplers"];
    if (!samplers.IsArray()) {
      LOG(FATAL) << "Expected samplers array.";
      return;
    }

    for (rapidjson::Value::ConstValueIterator iter = samplers.Begin();
         iter != samplers.End(); ++iter) {
      const rapidjson::Value& sampler = *iter;
      if (sampler.HasMember("usage")) {
        if (sampler.HasMember("usage_per_channel")) {
          LOG(FATAL) << "Sampler cannot have both usage and usages_per_channel "
                        "defined.";
          return;
        }

        const std::string usage_string =
            "Texture_" + std::string(sampler["usage"].GetString());
        AddUnique(Hash(usage_string), &environment_flags, document);
      }
      if (sampler.HasMember("usage_per_channel")) {
        const auto& usage_per_channel = sampler["usage_per_channel"];
        if (usage_per_channel.IsArray()) {
          std::string usage_string = "Texture_";
          for (rapidjson::Value::ConstValueIterator iter =
                   usage_per_channel.Begin();
               iter != usage_per_channel.End(); ++iter) {
            usage_string += std::string(iter->GetString());
          }
          AddUnique(Hash(usage_string), &environment_flags, document);
        }
      }
    }
  }

  // Iterate over the inputs to add environment flags.
  if (snippet->HasMember("inputs")) {
    const auto& inputs = (*snippet)["inputs"];
    if (!inputs.IsArray()) {
      LOG(FATAL) << "Expected inputs array.";
      return;
    }

    for (rapidjson::Value::ConstValueIterator iter = inputs.Begin();
         iter != inputs.End(); ++iter) {
      const rapidjson::Value& input = *iter;
      if (!input.HasMember("usage")) {
        continue;
      }

      const auto& usage = input["usage"];
      if (usage == "Position") {
        AddUnique(kAttributeHashPosition, &environment_flags, document);
      } else if (usage == "Color") {
        AddUnique(kAttributeHashColor, &environment_flags, document);
      } else if (usage == "TexCoord") {
        AddUnique(kAttributeHashUV, &environment_flags, document);
      } else if (usage == "Normal") {
        AddUnique(kAttributeHashNormal, &environment_flags, document);
      } else if (usage == "Tangent") {
        AddUnique(kAttributeHashTangent, &environment_flags, document);
      } else if (usage == "Orientation") {
        AddUnique(kAttributeHashOrientation, &environment_flags, document);
      } else if (usage == "BoneIndices") {
        AddUnique(kAttributeHashBoneIndices, &environment_flags, document);
      } else if (usage == "BoneWeights") {
        AddUnique(kAttributeHashBoneWeights, &environment_flags, document);
      }
    }
  }
}

size_t UniformTypeToNumElements(const std::string& type) {
  if (type == "Float1") {
    return 1;
  } else if (type == "Float2") {
    return 2;
  } else if (type == "Float3") {
    return 3;
  } else if (type == "Float4") {
    return 4;
  } else if (type == "Float2x2") {
    return 4;
  } else if (type == "Float3x3") {
    return 9;
  } else if (type == "Float4x4") {
    return 16;
  } else if (type == "Int1") {
    return 1;
  } else if (type == "Int2") {
    return 2;
  } else if (type == "Int3") {
    return 3;
  } else if (type == "Int4") {
    return 4;
  } else {
    LOG(FATAL) << "Uniform type " << type
               << " is either unsupported or doesn't have a generic size.";
    return 0;
  }
}

void ValidateUniformDataSize(const std::string& name, const std::string& type,
                             size_t count, size_t num_data) {
  const size_t expected_elements =
      UniformTypeToNumElements(type) * std::max(count, size_t(1));
  if (expected_elements != num_data) {
    LOG(FATAL) << "Uniform " << name << " of type " << type << " has "
               << num_data << " values, but expected " << expected_elements;
  }
}

bool ValidateUniformDataType(const rapidjson::Value& uniform,
                             bool top_level = true) {
  if (!uniform.HasMember("name")) {
    LOG(FATAL) << "Uniform must have a name.";
    return false;
  }
  if (!uniform.HasMember("type")) {
    LOG(FATAL) << "Uniform must have a type.";
    return false;
  }
  const std::string name = uniform["name"].GetString();
  const std::string type = uniform["type"].GetString();

  if (type == "Float1" || type == "Float2" || type == "Float3" ||
      type == "Float4" || type == "Int1" || type == "Int2" || type == "Int3" ||
      type == "Int4" || type == "Float2x2" || type == "Float3x3" ||
      type == "Float4x4") {
    if (uniform.HasMember("fields")) {
      LOG(FATAL) << "Uniform \"" << name << "\" of data type " << type
                 << " cannot have fields!";
      return false;
    }
  } else if (type == "Sampler2D") {
    if (!top_level) {
      LOG(FATAL) << "Uniform \"" << name << "\" of data type " << type
                 << " can only be a top level uniform!";
      return false;
    }
    if (uniform.HasMember("fields")) {
      LOG(FATAL) << "Uniform \"" << name << "\" of data type " << type
                 << " cannot have fields!";
      return false;
    }
  } else if (type == "Struct" || type == "BufferObject" ||
             type == "StorageBufferObject") {
    if (!top_level) {
      LOG(FATAL) << "Uniform \"" << name << "\" of data type " << type
                 << " can only be a top level uniform!";
      return false;
    }
    if (uniform.HasMember("array_size")) {
      LOG(FATAL) << "Uniforms of type 'Struct', 'BufferObject' and "
                    "'StorageBufferObject' cannot be an array.";
      return false;
    }
    if (uniform.HasMember("fields")) {
      auto& fields = uniform["fields"];
      if (!fields.IsArray()) {
        LOG(FATAL) << "Expected fields array.";
        return false;
      }
      for (rapidjson::Value::ConstValueIterator iter = fields.Begin();
           iter != fields.End(); ++iter) {
        const rapidjson::Value& field = *iter;
        if (!ValidateUniformDataType(field, false)) {
          return false;
        }
      }
    }
  }
  return true;
}

// Validates uniform values are correct and does processing as needed.
void ValidateAndProcessUniforms(rapidjson::Value* snippet,
                                rapidjson::Document* document) {
  if (!snippet || !document) {
    return;
  }

  if (!snippet->HasMember("uniforms")) {
    // No processing needed.
    return;
  }

  // Iterate over the uniforms.
  auto& uniforms = (*snippet)["uniforms"];
  if (!uniforms.IsArray()) {
    LOG(FATAL) << "Expected uniforms array.";
    return;
  }

  for (rapidjson::Value::ValueIterator iter = uniforms.Begin();
       iter != uniforms.End(); ++iter) {
    rapidjson::Value& uniform = *iter;

    if (!ValidateUniformDataType(uniform)) {
      LOG(FATAL) << "Unsupported uniform data type.";
      return;
    }
    const std::string name = uniform["name"].GetString();
    const std::string type = uniform["type"].GetString();

    const size_t array_size =
        uniform.HasMember("array_size") ? uniform["array_size"].GetUint() : 1;

    if (uniform.HasMember("values")) {
      const auto& values = uniform["values"];
      if (!values.IsArray()) {
        LOG(FATAL) << "Expected values array.";
        return;
      }
      ValidateUniformDataSize(name, type, array_size, values.Size());
      if (type == "Float1" || type == "Float2" || type == "Float3" ||
          type == "Float4" || type == "Float2x2" || type == "Float3x3" ||
          type == "Float4x4") {
        continue;
      } else if (type == "Int1" || type == "Int2" || type == "Int3" ||
                 type == "Int4") {
        // Create an int uniform array instead.
        rapidjson::Value int_uniforms(rapidjson::kArrayType);
        for (rapidjson::Value::ConstValueIterator value = values.Begin();
             value != values.End(); ++value) {
          const int int_value = value->IsInt()
                                    ? value->GetInt()
                                    : static_cast<int>(value->GetFloat());
          int_uniforms.PushBack(rapidjson::Value().SetInt(int_value),
                                document->GetAllocator());
        }
        uniform.AddMember("values_int", int_uniforms.Move(),
                          document->GetAllocator());
        uniform.RemoveMember("values");
      } else {
        LOG(FATAL) << "Unsupported default values for uniform " << name
                   << " with type " << type;
      }
    } else if (uniform.HasMember("values_int")) {
      if (type == "Int1" || type == "Int2" || type == "Int3" ||
          type == "Int4") {
        const auto& values = uniform["values_int"];
        if (!values.IsArray()) {
          LOG(FATAL) << "Expected values array.";
          return;
        }
        ValidateUniformDataSize(name, type, array_size, values.Size());
      } else {
        LOG(FATAL) << "Uniform " << name
                   << " has values_int, but is not of int type.";
      }
    }
  }
}

/// Helper function to process the code section of a json shader snippet.
void ProcessSnippetCodeSection(string_view field_name,
                               rapidjson::Value* snippet,
                               rapidjson::Document* document) {
  const std::string field_name_string(field_name);
  if (!snippet || !snippet->HasMember(field_name_string.c_str())) {
    return;
  }

  auto& json_string = (*snippet)[field_name_string.c_str()];
  std::string code_string = json_string.GetString();
  if (!ProcessShaderSource(&code_string)) {
    LOG(FATAL) << "Failed to process shader code snippet.";
  }
  json_string.SetString(code_string.c_str(), code_string.size(),
                        document->GetAllocator());
}

/// Helper function to add snippets from a json document to an existing
/// snippets array.
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
    CheckForVersion(snippet, &json);
    CreateSnippetEnvironmentFlags(snippet, &json);
    ValidateAndProcessUniforms(snippet, &json);
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
      rapidjson::Value().SetString(stage_name.data(), stage_name.size()),
      *allocator);
  stage.AddMember("snippets", snippets_jarray.Move(), *allocator);

  // Add the stage to the output array object.
  stages_array->PushBack(stage, *allocator);

  return true;
}
}  // namespace

Expected<std::string> BuildShaderJsonString(const ShaderBuildParams& params) {
  rapidjson::Document json;
  auto& object = json.SetObject();
  rapidjson::Value stages(rapidjson::kArrayType);

  // Process the different snippet types and add them to the stages object.
  // Order of inclusion is important for runtime processing. Order should be:
  // - Geometry stage.
  // - Vertex stage.
  // - Tessellation stage.
  // - Fragment stage.
  if (!CreateStageFromSnippetFiles(params.vertex_stages, "Vertex", &stages,
                                   &json.GetAllocator())) {
    return Expected<std::string>(
        LULL_ERROR(kErrorCode_Unknown, "Vertex stage creation failed"));
  }
  if (!CreateStageFromSnippetFiles(params.fragment_stages, "Fragment", &stages,
                                   &json.GetAllocator())) {
    return Expected<std::string>(
        LULL_ERROR(kErrorCode_Unknown, "Fragment stage creation failed"));
  }

  // Add the stages object to the main object.
  object.AddMember("stages", stages.Move(), json.GetAllocator());

  // Write the jsonnet into a json.
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  json.Accept(writer);

  return Expected<std::string>(buffer.GetString());
}

flatbuffers::DetachedBuffer BuildFlatBufferFromShaderJsonString(
    const std::string& shader_json_string, const ShaderBuildParams& params) {
  static const char* kShaderSchemaType = "lull.ShaderDef";
  return tool::JsonToFlatbuffer(shader_json_string.c_str(),
                                params.shader_schema_file_path,
                                kShaderSchemaType);
}

}  // namespace tool
}  // namespace lull
