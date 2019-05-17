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

#ifndef LULLABY_SYSTEMS_RENDER_FILAMENT_SHADER_MATERIAL_BUILDER_H_
#define LULLABY_SYSTEMS_RENDER_FILAMENT_SHADER_MATERIAL_BUILDER_H_

#include <string>
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/render/shader_description.h"
#include "lullaby/modules/render/shader_snippets_selector.h"
#include "lullaby/systems/render/filament/renderer.h"

namespace lull {

// Utility class that creates filament::Materials (and a corresponding
// ShaderDescription) using the provided ShaderSelectionParams.
class ShaderMaterialBuilder {
 public:
  // Builds a filament Material using the provided shader_def.
  ShaderMaterialBuilder(filament::Engine* engine,
                        const std::string& shading_model,
                        const ShaderDefT* shader_def,
                        const ShaderSelectionParams& params);

  // Builds a filament Material using the provided matc binary.
  ShaderMaterialBuilder(filament::Engine* engine,
                        const std::string& shading_model,
                        const void* raw_matc_data, size_t num_bytes);

  // Returns true if a filament::Material was successfully built.
  bool IsValid() const;

  // Returns the ShaderDescription that describes the built filament Material.
  const ShaderDescription& GetDescription() const;

  // Returns the shader selection params being used to generate the filament
  // Material.
  const ShaderSelectionParams* GetShaderSelectionParams() const;

  // Returns the filament Material that was built based on the inputs.
  filament::Material* GetFilamentMaterial();

  // Internal utitility class for code generation, made public for testing.
  static std::string BuildFragmentCode(const ShaderStage& stage);
  static std::string BuildFragmentCode(const ShaderDescription& desc);

 private:
  ShaderDescription description_;
  filament::Engine* engine_ = nullptr;
  filament::Material* material_ = nullptr;
  const ShaderSelectionParams* selection_params_ = nullptr;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_FILAMENT_SHADER_MATERIAL_BUILDER_H_
