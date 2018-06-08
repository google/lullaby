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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_SHADER_DATA_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_SHADER_DATA_H_

#include "lullaby/systems/render/next/shader.h"
#include "lullaby/util/optional.h"
#include "lullaby/util/string_view.h"
#include "lullaby/generated/shader_def_generated.h"

namespace lull {

// Params for loading shaders.
struct ShaderCreateParams {
  std::string shading_model;
  std::set<HashValue> environment;
  std::set<HashValue> features;

  ShaderCreateParams() = default;
  explicit ShaderCreateParams(string_view shading_model)
      : shading_model(shading_model) {}
};

/// Shader data used for creating shader programs.
class ShaderData {
 public:
  explicit ShaderData(const ShaderDefT& def);
  ShaderData(const ShaderDefT& def, const ShaderCreateParams& params);

  /// Returns true if the shader data contains a valid shader program.
  bool IsValid() const;

  /// Returns the shader description structure.
  const Shader::Description& GetDescription() const;
  /// Returns true if the shader contains the specified shader stage type.
  bool HasStage(ShaderStageType stage_type) const;
  /// Returns the code for a specific shader stage.
  string_view GetStageCode(ShaderStageType stage_type) const;

  /// Max number of stages in shader data.
  static const int kNumStages = ShaderStageType_MAX + 1;

 private:
  /// Build the data of this class from a ShaderDefT and create params.
  void BuildFromShaderDefT(const ShaderDefT& def,
                           const ShaderCreateParams& params);
  /// Gather inputs from snippet.
  bool GatherInputs(const ShaderSnippetDefT& snippet,
                    ShaderStageType stage_type);
  /// Gather outputs from snippet.
  bool GatherOutputs(const ShaderSnippetDefT& snippet,
                     ShaderStageType stage_type);
  /// Gather uniforms from snippet.
  bool GatherUniforms(const ShaderSnippetDefT& snippet,
                      ShaderStageType stage_type);
  /// Gather samplers from snippet.
  bool GatherSamplers(const ShaderSnippetDefT& snippet,
                      ShaderStageType stage_type);

  /// Is this data valid?
  bool is_valid_ = false;
  /// The shader description, including unique shader stage attributes and
  /// uniforms.
  Shader::Description description_;
  /// The code string for each shader stage.
  std::array<std::string, kNumStages> stage_code_;
  /// Input attribute defs for each shader stage.
  std::array<std::vector<ShaderAttributeDefT>, kNumStages> stage_inputs_;
  /// Output attribute defs for each shader stage.
  std::array<std::vector<ShaderAttributeDefT>, kNumStages> stage_outputs_;
  /// Uniform defs for each shader stage.
  std::array<std::vector<ShaderUniformDefT>, kNumStages> stage_uniforms_;
  /// Sampler defs for each shader stage.
  std::array<std::vector<ShaderSamplerDefT>, kNumStages> stage_samplers_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_SHADER_DATA_H_
