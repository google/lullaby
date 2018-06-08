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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_SHADER_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_SHADER_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "lullaby/systems/render/next/render_handle.h"
#include "lullaby/systems/render/next/uniform.h"
#include "lullaby/systems/render/shader.h"
#include "lullaby/util/span.h"
#include "lullaby/generated/material_def_generated.h"
#include "lullaby/generated/shader_def_generated.h"

namespace lull {

// Represents a shader program used for rendering.
class Shader {
 public:
  /// Description containing shader data included in the shader.
  struct Description {
    std::string shading_model;
    std::vector<ShaderUniformDefT> uniforms;
    std::vector<ShaderSamplerDefT> samplers;
    std::vector<ShaderAttributeDefT> attributes;

    Description() = default;
    explicit Description(string_view shading_model)
        : shading_model(shading_model) {}
  };

  /// Information to be used to bind textures to uniform samplers.
  struct Sampler {
    UniformHnd uniform;
    int texture_unit = -1;
  };

  explicit Shader(Description description)
      : description_(std::move(description)) {}
  Shader() {}
  ~Shader();

  Shader(const Shader& rhs) = delete;
  Shader& operator=(const Shader& rhs) = delete;

  // Locates the uniform in the shader with the specified |name|.
  UniformHnd FindUniform(const char* name);
  UniformHnd FindUniform(HashValue hash) const;

  // Sets the data for the specified uniform.
  bool SetUniform(HashValue name, const int* data, size_t len, int count = 1);
  bool SetUniform(HashValue name, const float* data, size_t len, int count = 1);
  bool SetUniform(UniformHnd id, const int* data, size_t len, int count = 1);
  bool SetUniform(UniformHnd id, const float* data, size_t len, int count = 1);

  // Binds the shader (ie. glUseProgram) for rendering.
  void Bind();

  // Returns the shader description structure.
  const Description& GetDescription() const;

  // Returns the sampler data associated with the texture usage.
  Sampler GetSampler(MaterialTextureUsage usage) const;

  // Binds a uniform.
  void BindUniform(UniformHnd hnd, const UniformData& uniform);
  void BindUniform(UniformHnd hnd, ShaderDataType type, Span<uint8_t> data);

 private:
  friend class ShaderFactory;
  void Init(ProgramHnd program, ShaderHnd vs, ShaderHnd fs);

  ProgramHnd program_;
  ShaderHnd vs_;
  ShaderHnd fs_;

  Description description_;

  std::unordered_map<HashValue, UniformHnd> uniforms_;
  Sampler samplers_[MaterialTextureUsage_MAX + 1];
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_SHADER_H_
