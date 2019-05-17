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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_SHADER_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_SHADER_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "lullaby/systems/render/detail/uniform_data.h"
#include "lullaby/modules/render/material_info.h"
#include "lullaby/modules/render/shader_description.h"
#include "lullaby/systems/render/next/render_handle.h"
#include "lullaby/systems/render/shader.h"
#include "lullaby/systems/render/texture.h"
#include "lullaby/util/span.h"
#include "lullaby/generated/material_def_generated.h"
#include "lullaby/generated/shader_def_generated.h"

namespace lull {

// Represents a shader program used for rendering.
class Shader {
 public:
  explicit Shader(ShaderDescription description)
      : description_(std::move(description)) {}
  Shader() {}
  ~Shader();

  Shader(const Shader& rhs) = delete;
  Shader& operator=(const Shader& rhs) = delete;

  bool IsUniformBlock(HashValue name) const;

  // Sets the data for the specified uniform.
  bool SetUniform(HashValue name, const int* data, size_t len, int count = 1);
  bool SetUniform(HashValue name, const float* data, size_t len, int count = 1);

  // Binds the shader (ie. glUseProgram) for rendering.
  void Bind();

  // Returns the shader description structure.
  const ShaderDescription& GetDescription() const;

  // Binds uniforms.
  void BindSampler(TextureUsageInfo usage, const TexturePtr& texture);
  void BindUniform(HashValue name, ShaderDataType type, Span<uint8_t> data);
  void BindUniformBlock(HashValue name, UniformBufferHnd ubo);
  void BindShaderUniformDef(const ShaderUniformDefT& uniform);
  void BindShaderSamplerDef(const ShaderSamplerDefT& sampler);

 private:
  friend class ShaderFactory;
  void Init(ProgramHnd program, ShaderHnd vs, ShaderHnd fs);

  UniformHnd FindUniform(HashValue hash) const;
  UniformHnd FindUniformBlock(HashValue hash) const;
  UniformBufferHnd GetDefaultUbo(const ShaderUniformDefT& uniform);
  void BindTexture(UniformHnd uniform, TextureHnd texture, int type, int unit);

  ProgramHnd program_;
  ShaderHnd vs_;
  ShaderHnd fs_;

  ShaderDescription description_;

  /// Information to be used to bind textures to uniform samplers.
  struct Sampler {
    UniformHnd uniform;
    int unit = -1;
  };


  std::unordered_map<HashValue, UniformHnd> uniforms_;
  std::unordered_map<HashValue, UniformHnd> uniform_blocks_;
  std::unordered_map<HashValue, UniformBufferHnd> default_ubos_;
  std::unordered_map<TextureUsageInfo, Sampler, TextureUsageInfo::Hasher>
      samplers_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_SHADER_H_
