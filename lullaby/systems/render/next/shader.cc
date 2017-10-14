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

#include "lullaby/systems/render/next/shader.h"

namespace lull {

void Shader::Init(fplbase::Renderer* renderer,
                  std::unique_ptr<fplbase::Shader> shader_impl) {
  renderer_ = renderer;
  impl_ = std::move(shader_impl);
}

Shader::UniformHnd Shader::FindUniform(const char* name) const {
  return impl_ ? impl_->FindUniform(name) : fplbase::InvalidUniformHandle();
}

void Shader::SetUniform(Shader::UniformHnd id, const float* value, size_t len) {
  if (impl_) {
    impl_->SetUniform(id, value, len);
  }
}

void Shader::Bind() {
  if (impl_ && renderer_) {
    renderer_->SetShader(impl_.get());
  }
}

}  // namespace lull
