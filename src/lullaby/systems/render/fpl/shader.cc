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

#include "lullaby/systems/render/fpl/shader.h"

namespace lull {

Shader::Shader(fplbase::Renderer* renderer, ShaderImplPtr shader)
    : impl_(std::move(shader)), renderer_(renderer) {}

Shader::UniformHnd Shader::FindUniform(const char* name) const {
  return impl_->FindUniform(name);
}

void Shader::SetUniform(Shader::UniformHnd id, const float* value, size_t len) {
  impl_->SetUniform(id, value, len);
}

void Shader::Bind() { renderer_->SetShader(impl_.get()); }

}  // namespace lull
