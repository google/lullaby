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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_SHADER_DATA_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_SHADER_DATA_H_

#include "lullaby/modules/render/shader_description.h"
#include "lullaby/modules/render/shader_snippets_selector.h"
#include "lullaby/util/optional.h"
#include "lullaby/util/string_view.h"
#include "lullaby/generated/shader_def_generated.h"

namespace lull {

// Params for loading shaders.
struct ShaderCreateParams {
  /// Name of the shading model.
  std::string shading_model;
  /// Selection params for picking snippets.
  ShaderSelectionParams selection_params;

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
  const ShaderDescription& GetDescription() const;
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

  /// Is this data valid?
  bool is_valid_ = false;
  /// The shader description, including unique shader stage attributes and
  /// uniforms.
  ShaderDescription description_;
  /// The code string for each shader stage.
  std::array<std::string, kNumStages> stage_code_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_SHADER_DATA_H_
