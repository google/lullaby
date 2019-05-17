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

#include "lullaby/systems/render/render_helpers.h"

#include "mathfu/glsl_mappings.h"

namespace lull {

void SetAlphaMultiplierDescendants(Entity entity, float alpha_multiplier,
                                   const TransformSystem* transform_system,
                                   RenderSystem* render_system) {
  const auto fn = [alpha_multiplier, render_system](Entity child) {
    mathfu::vec4 color;
    const bool use_default = !render_system->GetColor(child, &color);
    const mathfu::vec4 default_color = render_system->GetDefaultColor(child);
    color[0] = use_default ? default_color[0] : color[0];
    color[1] = use_default ? default_color[1] : color[1];
    color[2] = use_default ? default_color[2] : color[2];
    color[3] = default_color[3] * alpha_multiplier;
    render_system->SetColor(child, color);
  };

  transform_system->ForAllDescendants(entity, fn);
}

void SetRenderPassDescendants(Entity entity, RenderPass pass,
                              const TransformSystem* transform_system,
                              RenderSystem* render_system) {
  const auto fn = [pass, render_system](Entity child) {
    render_system->SetRenderPass(child, pass);
  };

  transform_system->ForAllDescendants(entity, fn);
}

mathfu::mat4 CalculateClipFromModelMatrix(const mathfu::mat4& model,
                                          const mathfu::mat4& projection_view) {
  return projection_view * model;
}

HashValue FixRenderPass(HashValue pass) {
  if (pass == ConstHash("Pano")) {
    return static_cast<HashValue>(RenderPass_Pano);
  } else if (pass == ConstHash("Opaque")) {
    return static_cast<HashValue>(RenderPass_Opaque);
  } else if (pass == ConstHash("Main")) {
    return static_cast<HashValue>(RenderPass_Main);
  } else if (pass == ConstHash("OverDraw")) {
    return static_cast<HashValue>(RenderPass_OverDraw);
  } else if (pass == ConstHash("Debug")) {
    return static_cast<HashValue>(RenderPass_Debug);
  } else if (pass == ConstHash("Invisible")) {
    return static_cast<HashValue>(RenderPass_Invisible);
  } else if (pass == ConstHash("OverDrawGlow")) {
    return static_cast<HashValue>(RenderPass_OverDrawGlow);
  }

  return pass;
}

ShaderDataType FloatDimensionsToUniformType(int dimensions) {
  switch (dimensions) {
    case 1:
      return ShaderDataType_Float1;
    case 2:
      return ShaderDataType_Float2;
    case 3:
      return ShaderDataType_Float3;
    case 4:
      return ShaderDataType_Float4;
    case 9:
      return ShaderDataType_Float3x3;
    case 16:
      return ShaderDataType_Float4x4;
    default:
      LOG(DFATAL) << "Failed to convert dimensions to uniform type.";
      return ShaderDataType_Float1;
  }
}

ShaderDataType IntDimensionsToUniformType(int dimensions) {
  switch (dimensions) {
    case 1:
      return ShaderDataType_Int1;
    case 2:
      return ShaderDataType_Int2;
    case 3:
      return ShaderDataType_Int3;
    case 4:
      return ShaderDataType_Int4;
    default:
      LOG(DFATAL) << "Failed to convert dimensions to uniform type.";
      return ShaderDataType_Int1;
  }
}

void ClearBoneTransforms(RenderSystem* render_system, Entity entity,
                         int bone_count) {
  constexpr const char* kBoneTransformsUniform = "bone_transforms";
  constexpr int kDimension = 4;
  constexpr int kNumVec4sInAffineTransform = 3;
  static const mathfu::AffineTransform identity =
      mathfu::mat4::ToAffineTransform(mathfu::mat4::Identity());
  static std::vector<mathfu::AffineTransform> bones(kMaxNumBones, identity);

  const int count = kNumVec4sInAffineTransform * bone_count;
  const float* data = &(bones[0][0]);
  render_system->SetUniform(entity, kBoneTransformsUniform, data,
                            kDimension, count);
}

}  // namespace lull
