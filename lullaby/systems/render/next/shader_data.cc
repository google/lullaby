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

#include "lullaby/systems/render/next/shader_data.h"

#include "lullaby/util/fixed_string.h"

namespace lull {
using MetadataString = FixedString<256>;

namespace {
bool ShaderUniformDefsEquals(const ShaderUniformDefT& lhs,
                             const ShaderUniformDefT& rhs) {
  if (lhs.type != rhs.type) {
    return false;
  } else if (lhs.array_size != rhs.array_size) {
    return false;
  } else if (lhs.fields.size() != rhs.fields.size()) {
    return false;
  }
  for (size_t i = 0; i < lhs.fields.size(); ++i) {
    if (!ShaderUniformDefsEquals(lhs.fields[i], rhs.fields[i])) {
      return false;
    }
  }
  return true;
}

bool ShaderSamplerDefsEqual(const ShaderSamplerDefT& lhs,
                            const ShaderSamplerDefT& rhs) {
  return lhs.usage == rhs.usage && lhs.type == rhs.type;
}

bool CompareShaderAttributeDefs(const ShaderAttributeDefT& lhs,
                                const ShaderAttributeDefT& rhs) {
  return lhs.type == rhs.type && lhs.usage == rhs.usage;
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
      LOG(DFATAL) << "ShaderDataType_BufferObject not yet supported.";
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

MetadataString ConstructShaderAttributeInputString(
    const ShaderAttributeDefT& def, ShaderStageType stage) {
  if (def.type == VertexAttributeType_Empty) {
    return "";
  }

  MetadataString str_type =
      VertexAttributeTypeToString(def.type) + " " + def.name.c_str() + ";\n";
  if (stage == ShaderStageType_Vertex) {
    return MetadataString("attribute ") + str_type;
  } else if (stage == ShaderStageType_Fragment) {
    return MetadataString("varying ") + str_type;
  }

  LOG(DFATAL) << "Unsupported shader stage: " << stage;
  return "";
}

MetadataString ConstructShaderAttributeOutputString(
    const ShaderAttributeDefT& def) {
  MetadataString str_type =
      VertexAttributeTypeToString(def.type) + " " + def.name + ";\n";
  return MetadataString("varying ") + str_type;
}

MetadataString ConstructShaderUniformString(const ShaderUniformDefT& def) {
  MetadataString uniform_string = MetadataString("uniform ") +
                                  ShaderDataTypeToString(def.type) + " " +
                                  def.name;
  if (def.array_size) {
    uniform_string += "[" + std::to_string(def.array_size) + "]";
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

// Validates that a uniform def does not conflict an existing one and adds it
// if one does not already exist.
// Returns false on validation failure.
bool ValidateAndAddUniformDef(const ShaderUniformDefT& uniform,
                              std::vector<ShaderUniformDefT>* uniforms) {
  if (!uniforms) {
    LOG(DFATAL) << "Did not supply uniforms vector.";
    return false;
  }

  auto iter = std::find_if(
      uniforms->begin(), uniforms->end(),
      [&](const ShaderUniformDefT& def) { return def.name == uniform.name; });
  if (iter == uniforms->end()) {
    uniforms->push_back(uniform);
  } else if (!ShaderUniformDefsEquals(*iter, uniform)) {
    LOG(DFATAL) << "Snippets contain uniforms with same name (" << uniform.name
                << ") but different definitions.";
    return false;
  }
  return true;
}

// Validates that an attribute def does not conflict an existing one and adds
// it if one does not already exist.
// Returns false on validation failure.
bool ValidateAndAddAttributeDef(const ShaderAttributeDefT& attribute,
                                std::vector<ShaderAttributeDefT>* attributes) {
  if (!attributes) {
    LOG(DFATAL) << "Did not supply attributes vector.";
    return false;
  }

  auto iter = std::find_if(attributes->begin(), attributes->end(),
                           [&](const ShaderAttributeDefT& def) {
                             return def.name == attribute.name;
                           });
  if (iter == attributes->end()) {
    attributes->push_back(attribute);
  } else if (!CompareShaderAttributeDefs(*iter, attribute)) {
    LOG(DFATAL) << "Snippets contain attributes with same name ("
                << attribute.name << ") but different definitions.";
    return false;
  }
  return true;
}

// Validates that a sampler def does not conflict an existing one and adds it
// if one does not already exist.
// Returns false on validation failure.
bool ValidateAndAddSamplerDef(const ShaderSamplerDefT& sampler,
                              std::vector<ShaderSamplerDefT>* samplers) {
  if (!samplers) {
    LOG(DFATAL) << "Did not supply samplers vector.";
    return false;
  }

  auto iter = std::find_if(
      samplers->begin(), samplers->end(),
      [&](const ShaderSamplerDefT& def) { return def.name == sampler.name; });
  if (iter == samplers->end()) {
    samplers->push_back(sampler);
  } else if (!ShaderSamplerDefsEqual(*iter, sampler)) {
    LOG(DFATAL) << "Snippets contain samplers with same name (" << sampler.name
                << ") but different definitions.";
    return false;
  }

  return true;
}

MetadataString GenerateFunctionName(ShaderStageType stage,
                                    size_t function_index) {
  return MetadataString("GeneratedFunction") + EnumNameShaderStageType(stage) +
         std::to_string(function_index);
}

bool IsSubsetInSuperset(std::vector<HashValue> subset,
                        const std::set<HashValue>& superset) {
  std::sort(subset.begin(), subset.end());
  return std::includes(superset.cbegin(), superset.cend(), subset.cbegin(),
                       subset.cend());
}

std::vector<const ShaderSnippetDefT*> FindSupportedSnippets(
    const ShaderCreateParams& params, const ShaderStageDefT& stage) {
  std::vector<const ShaderSnippetDefT*> supported_snippets;
  std::set<HashValue> features_to_support = params.features;
  for (const auto& snippet : stage.snippets) {
    if (!IsSubsetInSuperset(snippet.environment, params.environment)) {
      continue;
    }
    if (IsSubsetInSuperset(snippet.features, features_to_support)) {
      // Add this snippet to be evaluated further.
      supported_snippets.push_back(&snippet);

      // Remove the features added by the snippet. This guarantees features
      // are only added once per shader stage.
      for (const auto& feature : snippet.features) {
        features_to_support.erase(feature);
      }
    }
  }
  return supported_snippets;
}

bool ValidateSnippetInputsIncluded(
    const ShaderSnippetDefT& snippet,
    const std::vector<const ShaderSnippetDefT*>& snippets) {
  // Make sure that each input in the snippet is included at least once as an
  // output in the snippets vector.
  for (const auto& input : snippet.inputs) {
    bool found = false;
    for (const auto& snippet_iter : snippets) {
      // Is the input included in one of this snippet's outputs?
      if (std::find_if(
              snippet_iter->outputs.cbegin(), snippet_iter->outputs.cend(),
              [input](const ShaderAttributeDefT& def) {
                return (input.name == def.name && input.type == def.type);
              }) != snippet_iter->outputs.cend()) {
        // Found an output corresponding to the input. Break and move to the
        // next input.
        found = true;
        break;
      }
    }
    if (!found) {
      // An input was not found. Return false.
      return false;
    }
  }
  // All inputs have been found.
  return true;
}

bool ValidateSnippetOutputsIncluded(
    const ShaderSnippetDefT& snippet,
    const std::vector<const ShaderSnippetDefT*>& snippets) {
  // Make sure that each output in the snippet is included at least once as an
  // input in the snippets vector.
  for (const auto& output : snippet.outputs) {
    bool found = false;
    for (const auto& snippet_iter : snippets) {
      // Is the output included in one of this snippet's input?
      if (std::find_if(
              snippet_iter->inputs.cbegin(), snippet_iter->inputs.cend(),
              [output](const ShaderAttributeDefT& def) {
                return (output.name == def.name && output.type == def.type);
              }) != snippet_iter->inputs.cend()) {
        // Found an input corresponding to the output. Break and move to the
        // next output.
        found = true;
        break;
      }
    }
    if (!found) {
      // An output was not found. Return false.
      return false;
    }
  }
  // All outputs have been found.
  return true;
}

#ifdef SHADER_DEBUG
void PrintSnippetsNames(string_view prefix_string,
                        const std::vector<const ShaderSnippetDefT*>& snippets) {
  if (!snippets.empty()) {
    bool first = true;
    std::stringstream ss;
    ss << prefix_string.c_str();
    for (const auto& it : snippets) {
      if (!first) {
        ss << ", ";
      }
      ss << it->name;
      first = false;
    }
    LOG(INFO) << ss.str() << ".";
  }
}
#endif

}  // namespace

ShaderData::ShaderData(const ShaderDefT& def) {
  const ShaderCreateParams params;
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

const Shader::Description& ShaderData::GetDescription() const {
  return description_;
}

void ShaderData::BuildFromShaderDefT(const ShaderDefT& def,
                                     const ShaderCreateParams& params) {
  is_valid_ = false;

  // Find supported snippets from each stage.
  std::vector<const ShaderSnippetDefT*> supported_snippets[kNumStages];
  for (const auto& stage : def.stages) {
    supported_snippets[stage.type] = FindSupportedSnippets(params, stage);
  }

  // Remove mismatching inputs/outputs.
  bool first_nonempty_stage = true;
  for (int stage_index = 0; stage_index < kNumStages; ++stage_index) {
    std::vector<const ShaderSnippetDefT*>& current_stage =
        supported_snippets[stage_index];
    auto is_invalid_snippet = [first_nonempty_stage, &supported_snippets,
                               stage_index](const ShaderSnippetDefT* snippet) {
      if (snippet == nullptr) {
        return true;
      }
      // Check inputs of this snippet are included as outputs of snippets in the
      // previous stage (skip if this is the first non-empty shader stage).
      if (!first_nonempty_stage &&
          !ValidateSnippetInputsIncluded(*snippet,
                                         supported_snippets[stage_index - 1])) {
        return true;
      }
      // Check that outputs of this snippet are included as outputs of next
      // stage (unless this is the final stage. The final shader stage is always
      // ShaderStageType_MAX [= ShaderStageType_FRAGMENT] so we only need to
      // make sure we don't attempt to access beyond that.
      if (stage_index < ShaderStageType_MAX &&
          !ValidateSnippetOutputsIncluded(
              *snippet, supported_snippets[stage_index + 1])) {
        return true;
      }
      return false;
    };
    current_stage.erase(std::remove_if(current_stage.begin(),
                                       current_stage.end(), is_invalid_snippet),
                        current_stage.end());

    if (!current_stage.empty()) {
      first_nonempty_stage = false;
    }
  }

  // Construct the stages from final snippets.
  for (int stage_index = ShaderStageType_MIN; stage_index < kNumStages;
       ++stage_index) {
    const ShaderStageType stage_type =
        static_cast<ShaderStageType>(stage_index);
    std::string stage_src = "";
    std::vector<MetadataString> stage_functions;
    stage_functions.reserve(supported_snippets[stage_index].size());
    for (const auto& iter : supported_snippets[stage_index]) {
      const ShaderSnippetDefT& snippet = *iter;
      // Construct the source code for the stage.
      if (!snippet.code.empty()) {
        stage_src.append(snippet.code);
        if (stage_src.back() != '\n') {
          stage_src.append("\n");
        }
      }
      if (!snippet.main_code.empty()) {
        const MetadataString generated_function_name =
            GenerateFunctionName(stage_type, stage_functions.size());
        stage_functions.push_back(generated_function_name);
        std::stringstream function_code;
        function_code << "void " << generated_function_name << "() {\n"
                      << snippet.main_code;
        if (snippet.main_code.back() != '\n') {
          function_code << "\n";
        }
        function_code << "}\n";
        stage_src.append(function_code.str());
      }

      if (!GatherInputs(snippet, stage_type)) {
        return;
      }
      if (!GatherOutputs(snippet, stage_type)) {
        return;
      }
      if (!GatherUniforms(snippet, stage_type)) {
        return;
      }
      if (!GatherSamplers(snippet, stage_type)) {
        return;
      }
    }

    // Concatenate the uniforms and attribute code strings.
    std::stringstream code;
    for (const auto& uniform : stage_uniforms_[stage_index]) {
      code << ConstructShaderUniformString(uniform);
    }

    // Construct the samplers.
    for (const auto& sampler : stage_samplers_[stage_index]) {
      code << ConstructShaderSamplerString(sampler);
    }

    // Construct the input attributes.
    for (const auto& attribute : stage_inputs_[stage_index]) {
      code << ConstructShaderAttributeInputString(attribute, stage_type);
    }

    // Generate code string for output variables if needed.
    for (const auto& attribute : stage_outputs_[stage_index]) {
      code << ConstructShaderAttributeOutputString(attribute);
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
    for (const auto& uniform : stage_uniforms_[stage_index]) {
      if (!ValidateAndAddUniformDef(uniform, &description_.uniforms)) {
        return;
      }
    }

    for (const auto& sampler : stage_samplers_[stage_index]) {
      if (!ValidateAndAddSamplerDef(sampler, &description_.samplers)) {
        return;
      }
    }
  }

  // Copy the vertex stage inputs to the shader description.
  for (const auto& input : stage_outputs_[ShaderStageType_Vertex]) {
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
                       supported_snippets[i]);
  }
#endif
}

bool ShaderData::GatherInputs(const ShaderSnippetDefT& snippet,
                              ShaderStageType stage_type) {
  // Add the input to the inputs list.
  for (const auto& input : snippet.inputs) {
    // Check for an existing input with the same name and that they match in
    // definition.
    if (!ValidateAndAddAttributeDef(input, &stage_inputs_[stage_type])) {
      LOG(DFATAL) << "Input validation failed.";
      return false;
    }
  }
  return true;
}

bool ShaderData::GatherOutputs(const ShaderSnippetDefT& snippet,
                               ShaderStageType stage_type) {
  // Add the input to the outputs list.
  for (const auto& output : snippet.outputs) {
    // Check for an existing input with the same name and that they match in
    // definition.
    if (!ValidateAndAddAttributeDef(output, &stage_outputs_[stage_type])) {
      LOG(DFATAL) << "Output validation failed.";
      return false;
    }
  }
  return true;
}

bool ShaderData::GatherUniforms(const ShaderSnippetDefT& snippet,
                                ShaderStageType stage_type) {
  // Add the uniforms to the shader description.
  for (const auto& uniform : snippet.uniforms) {
    // Check for an existing uniform with the same name and that they match
    // in definition.
    if (!ValidateAndAddUniformDef(uniform, &stage_uniforms_[stage_type])) {
      LOG(DFATAL) << "Uniform validation failed.";
      return false;
    }
  }
  return true;
}

bool ShaderData::GatherSamplers(const ShaderSnippetDefT& snippet,
                                ShaderStageType stage_type) {
  // Add the uniforms to the shader description.
  for (const auto& sampler : snippet.samplers) {
    // Check for an existing sampler with the same name and that they match
    // in definition.
    if (!ValidateAndAddSamplerDef(sampler, &stage_samplers_[stage_type])) {
      LOG(DFATAL) << "Sampler validation failed.";
      return false;
    }
  }
  return true;
}

}  // namespace lull
