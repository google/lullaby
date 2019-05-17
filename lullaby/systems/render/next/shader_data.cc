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

#include "lullaby/systems/render/next/shader_data.h"

#include <algorithm>
#include "lullaby/generated/flatbuffers/shader_def_generated.h"
#include "lullaby/modules/render/sanitize_shader_source.h"
#include "lullaby/modules/render/shader_snippets_selector.h"
#include "lullaby/util/fixed_string.h"

namespace lull {
using MetadataString = FixedString<256>;

namespace {
static constexpr int kUnspecifiedVersion = 0;
static const char kCompatibilityShaderMacros[] = R"(#define UNIFORM(X) X
#define SAMPLER(X) X
)";

MetadataString ConstructShaderVersionString(ShaderLanguage shader_lang,
                                            int version) {
  if (version == kUnspecifiedVersion) {
    LOG(DFATAL) << "Shader version must be specified.";
    version = GetMinimumShaderVersion(shader_lang);
  }

  if (shader_lang == ShaderLanguage_GLSL) {
    return MetadataString("#version ") + std::to_string(version) + "\n";
  } else {
    return MetadataString("#version ") + std::to_string(version) + " es\n";
  }
}

MetadataString VertexAttributeTypeToString(VertexAttributeType type) {
  switch (type) {
    case VertexAttributeType_Empty:
      LOG(DFATAL) << "Empty vertex attribute.";
      return "";
    case VertexAttributeType_Scalar1f:
      return "float";
    case VertexAttributeType_Vec2f:
      return "vec2";
    case VertexAttributeType_Vec3f:
      return "vec3";
    case VertexAttributeType_Vec4f:
      return "vec4";
    case VertexAttributeType_Vec2us:
      return "uvec2";
    case VertexAttributeType_Vec4us:
      return "uvec4";
    case VertexAttributeType_Vec4ub:
      return "bvec4";
  }
}

MetadataString ShaderDataTypeToString(ShaderDataType type) {
  switch (type) {
    case ShaderDataType_Float1:
      return "float";
    case ShaderDataType_Float2:
      return "vec2";
    case ShaderDataType_Float3:
      return "vec3";
    case ShaderDataType_Float4:
      return "vec4";
    case ShaderDataType_Int1:
      return "int";
    case ShaderDataType_Int2:
      return "ivec2";
    case ShaderDataType_Int3:
      return "ivec3";
    case ShaderDataType_Int4:
      return "ivec4";
    case ShaderDataType_Float2x2:
      return "mat2";
    case ShaderDataType_Float3x3:
      return "mat3";
    case ShaderDataType_Float4x4:
      return "mat4";
    case ShaderDataType_Sampler2D:
      return "sampler2D";
    case ShaderDataType_Struct:
      LOG(DFATAL) << "ShaderDataType_Struct not yet supported.";
      return "";
    case ShaderDataType_BufferObject:
      LOG(DFATAL)
          << "ShaderDataType_BufferObject should not go through this function.";
      return "";
    case ShaderDataType_StorageBufferObject:
      LOG(DFATAL) << "ShaderDataType_StorageBufferObject not yet supported.";
      return "";
  }
}

MetadataString TextureTargetTypeToString(TextureTargetType type) {
  switch (type) {
    case TextureTargetType_Standard2d:
      return "sampler2D";
    case TextureTargetType_CubeMap:
      return "samplerCube";
  }
}

bool IsInOutKeywordSupported(ShaderLanguage shader_language,
                             int shader_version) {
  if (shader_language == ShaderLanguage_GL_Compat ||
      shader_language == ShaderLanguage_GLSL_ES) {
    if (shader_version < 300) {
      return false;
    }
  } else if (shader_language == ShaderLanguage_GLSL) {
    if (shader_version < 130) {
      return false;
    }
  }
  return true;
}

MetadataString ConstructShaderAttributeInputString(
    const ShaderAttributeDefT& def, ShaderStageType stage,
    bool in_out_key_support) {
  if (def.type == VertexAttributeType_Empty) {
    return "";
  }
  MetadataString str_type =
      VertexAttributeTypeToString(def.type) + " " + def.name.c_str() + ";\n";

  if (in_out_key_support) {
    return MetadataString("in ") + str_type;
  } else if (stage == ShaderStageType_Vertex) {
    // No "in" keyword support.
    return MetadataString("attribute ") + str_type;
  } else if (stage == ShaderStageType_Fragment) {
    return MetadataString("varying ") + str_type;
  }

  LOG(DFATAL) << "Unsupported shader stage: " << stage;
  return "";
}

MetadataString ConstructShaderAttributeOutputString(
    const ShaderAttributeDefT& def, bool in_out_key_support) {
  MetadataString str_type =
      VertexAttributeTypeToString(def.type) + " " + def.name + ";\n";
  if (in_out_key_support) {
    return MetadataString("out ") + str_type;
  } else {
    return MetadataString("varying ") + str_type;
  }
}

MetadataString ConstructShaderUniformString(const ShaderUniformDefT& def) {
  MetadataString uniform_string;
  if (def.type == ShaderDataType_BufferObject) {
    uniform_string =
        MetadataString("layout (std140) uniform ") + def.name + " {\n";
    for (auto& it : def.fields) {
      uniform_string += ShaderDataTypeToString(it.type) + " " + it.name;
      if (it.array_size) {
        uniform_string += "[" + std::to_string(it.array_size) + "]";
      }
      uniform_string += ";\n";
    }
    uniform_string += "}";
  } else {
    uniform_string = MetadataString("uniform ") +
                     ShaderDataTypeToString(def.type) + " " + def.name;
    if (def.array_size) {
      uniform_string += "[" + std::to_string(def.array_size) + "]";
    }
  }
  uniform_string += ";\n";
  return uniform_string;
}

MetadataString ConstructShaderSamplerString(const ShaderSamplerDefT& def) {
  MetadataString uniform_string = MetadataString("uniform ") +
                                  TextureTargetTypeToString(def.type) + " " +
                                  def.name;
  uniform_string += ";\n";
  return uniform_string;
}

MetadataString GenerateFunctionName(ShaderStageType stage,
                                    size_t function_index) {
  return MetadataString("GeneratedFunction") + EnumNameShaderStageType(stage) +
         std::to_string(function_index);
}

#ifdef SHADER_DEBUG
void PrintSnippetsNames(string_view prefix_string, const ShaderStage& stage) {
  if (!stage.snippet_names.empty()) {
    bool first = true;
    std::stringstream ss;
    ss << prefix_string;
    for (const auto& it : stage.snippet_names) {
      if (!first) {
        ss << ", ";
      }
      ss << it;
      first = false;
    }
    LOG(INFO) << ss.str() << ".";
  }
}
#endif

}  // namespace

ShaderData::ShaderData(const ShaderDefT& def) {
  const ShaderCreateParams params{};
  BuildFromShaderDefT(def, params);
}

ShaderData::ShaderData(const ShaderDefT& def,
                       const ShaderCreateParams& params) {
  BuildFromShaderDefT(def, params);
}

bool ShaderData::IsValid() const { return is_valid_; }

bool ShaderData::HasStage(ShaderStageType stage_type) const {
  return is_valid_ && !stage_code_[stage_type].empty();
}

string_view ShaderData::GetStageCode(ShaderStageType stage_type) const {
  DCHECK(IsValid()) << "Called GetStageCode on an invalid shader.";
  return stage_code_[stage_type];
}

const ShaderDescription& ShaderData::GetDescription() const {
  return description_;
}

void ShaderData::BuildFromShaderDefT(const ShaderDefT& def,
                                     const ShaderCreateParams& params) {
  is_valid_ = false;

  SnippetSelectionResult selected_snippets =
      SelectShaderSnippets(def, params.selection_params);
  const bool is_input_key_supported = IsInOutKeywordSupported(
      params.selection_params.lang, selected_snippets.shader_version);

  // Construct the stages from final snippets.
  for (int stage_index = ShaderStageType_MIN; stage_index < kNumStages;
       ++stage_index) {
    const ShaderStage& stage = selected_snippets.stages[stage_index];
    const ShaderStageType stage_type =
        static_cast<ShaderStageType>(stage_index);

    std::string stage_src = "";
    std::vector<MetadataString> stage_functions;
    stage_functions.reserve(stage.main.size());

    for (const auto& code : stage.code) {
      // Construct the source code for the stage.
      if (!code.empty()) {
        stage_src.append(code.data(), code.length());
        if (code.back() != '\n') {
          stage_src.append("\n");
        }
      }
    }

    for (const auto& main_code : stage.main) {
      if (!main_code.empty()) {
        const MetadataString generated_function_name =
            GenerateFunctionName(stage_type, stage_functions.size());
        stage_functions.push_back(generated_function_name);
        std::stringstream function_code;
        function_code << "void " << generated_function_name << "() {\n"
                      << main_code;
        if (main_code.back() != '\n') {
          function_code << "\n";
        }
        function_code << "}\n";
        stage_src.append(function_code.str());
      }
    }

    if (stage_functions.empty() && stage_src.empty()) {
      continue;
    }

    // Concatenate the uniforms and attribute code strings.
    std::stringstream code;
    code << ConstructShaderVersionString(params.selection_params.lang,
                                         selected_snippets.shader_version);

    code << kCompatibilityShaderMacros;

    for (const auto& uniform : stage.uniforms) {
      code << ConstructShaderUniformString(uniform);
    }

    // Construct the samplers.
    for (const auto& sampler : stage.samplers) {
      code << ConstructShaderSamplerString(sampler);
    }

    // Construct the input attributes.
    for (const auto& attribute : stage.inputs) {
      code << ConstructShaderAttributeInputString(attribute, stage_type,
                                                  is_input_key_supported);
    }

    // Generate code string for output variables if needed.
    const bool skip_output_strings =
        stage_type == ShaderStageType_Fragment && !is_input_key_supported;
    if (!skip_output_strings) {
      for (const auto& attribute : stage.outputs) {
        code << ConstructShaderAttributeOutputString(attribute,
                                                     is_input_key_supported);
      }
    }

    // Add the metadata code to the shader stage code.
    code << stage_src;

    // Construct the main code.
    if (!stage_functions.empty()) {
      code << "\nvoid main() {\n";
      for (const auto& function : stage_functions) {
        code << function << "();\n";
      }
      code << "}\n";
    }
    stage_code_[stage_index] = code.str();

    // Copy the uniforms to the shader description.
    for (const auto& uniform : stage.uniforms) {
      if (!ValidateAndAddUniformDef(uniform, &description_.uniforms)) {
        return;
      }
    }

    for (const auto& sampler : stage.samplers) {
      if (!ValidateAndAddSamplerDef(sampler, &description_.samplers)) {
        return;
      }
    }
  }

  // Copy the vertex stage inputs to the shader description.
  for (const auto& input :
       selected_snippets.stages[ShaderStageType_Vertex].outputs) {
    if (!ValidateAndAddAttributeDef(input, &description_.attributes)) {
      return;
    }
  }

  // Success!
  description_.shading_model = params.shading_model;
  is_valid_ = true;

#ifdef SHADER_DEBUG
  // Print out selected snippets.
  for (int i = ShaderStageType_MIN; i <= ShaderStageType_MAX; ++i) {
    const std::string stage_string =
        EnumNameShaderStageType(static_cast<ShaderStageType>(i));
    PrintSnippetsNames("Selected snippets for " + stage_string + " stage: ",
                       selected_snippets.stages[i]);
  }
#endif
}

}  // namespace lull
