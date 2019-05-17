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

#ifndef LULLABY_MODULES_RENDER_SHADER_SNIPPETS_SELECTOR_H_
#define LULLABY_MODULES_RENDER_SHADER_SNIPPETS_SELECTOR_H_

#include <set>
#include <string>

#include "lullaby/modules/render/shader_description.h"
#include "lullaby/util/string_view.h"
#include "lullaby/generated/shader_def_generated.h"

namespace lull {

/// Max number of stages in shader data.
static constexpr int kNumShaderStages = ShaderStageType_MAX + 1;
struct ShaderStage;
using ShaderStagesArray = std::array<ShaderStage, kNumShaderStages>;

// Params for loading shaders.
struct ShaderSelectionParams {
  /// Shader language to look for when picking snippets.
  ShaderLanguage lang = ShaderLanguage_GL_Compat;
  /// Maximum shader version to compile to. 0 = All versions.
  int max_shader_version = 0;
  /// Flags supported by the runtime environment.
  std::set<HashValue> environment;
  /// Shader features requested.
  std::set<HashValue> features;
};

// Struct containing all the information which describes a shader stage.
struct ShaderStage {
  /// Names of snippets includes in this shader stage.
  std::vector<string_view> snippet_names;
  /// The header code string for shader stage. Note that the string_view refers
  /// to an external string which must remain in memory or else the code string
  /// views will be invalid.
  std::vector<string_view> code;
  /// The main code strings for the shader stage. Note that the string_view
  /// refers to an external string which must remain in memory or else the code
  /// string views will be invalid.
  std::vector<string_view> main;
  /// Input attribute defs for the shader stage.
  std::vector<ShaderAttributeDefT> inputs;
  /// Output attribute defs for the shader stage.
  std::vector<ShaderAttributeDefT> outputs;
  /// Uniform defs for the shader stage.
  std::vector<ShaderUniformDefT> uniforms;
  /// Sampler defs for the shader stage.
  std::vector<ShaderSamplerDefT> samplers;
};

// Result of shader snippets selection function.
struct SnippetSelectionResult {
  // Shader version for the selected shader.
  int shader_version = 0;

  // Find supported snippets from each stage.
  ShaderStagesArray stages;
};

// Returns the minimum shader version for the given shader language.
int GetMinimumShaderVersion(ShaderLanguage shader_lang);

// Selection shader snippets in accordance to selection params.
SnippetSelectionResult SelectShaderSnippets(
    const ShaderDefT& def, const ShaderSelectionParams& params);

// Validates that a uniform def does not conflict an existing one and adds it
// if one does not already exist.
// Returns false on validation failure.
bool ValidateAndAddUniformDef(const ShaderUniformDefT& uniform,
                              std::vector<ShaderUniformDefT>* uniforms);

// Validates that an attribute def does not conflict an existing one and adds
// it if one does not already exist.
// Returns false on validation failure.
bool ValidateAndAddAttributeDef(const ShaderAttributeDefT& attribute,
                                std::vector<ShaderAttributeDefT>* attributes);

// Validates that a sampler def does not conflict an existing one and adds it
// if one does not already exist.
// Returns false on validation failure.
bool ValidateAndAddSamplerDef(const ShaderSamplerDefT& sampler,
                              std::vector<ShaderSamplerDefT>* samplers);

// Utility to create a ShaderDescription from shader stages.
ShaderDescription CreateShaderDescription(string_view shading_model,
                                          const ShaderStagesArray& stages);

}  // namespace lull

#endif  // LULLABY_MODULES_RENDER_SHADER_SNIPPETS_SELECTOR_H_
