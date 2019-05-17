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

#include "lullaby/systems/render/fpl/shader.h"

#include "fplbase/glplatform.h"
#include "fplbase/internal/type_conversions_gl.h"

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

void Shader::BindUniform(const Uniform& uniform) const {
  const Uniform::Description& desc = uniform.GetDescription();
  int binding = 0;
  if (desc.binding >= 0) {
    binding = desc.binding;
  } else {
    const Shader::UniformHnd handle = FindUniform(desc.name.c_str());
    if (fplbase::ValidUniformHandle(handle)) {
      binding = fplbase::GlUniformHandle(handle);
    } else {
      // Material has a uniform which is not present in the shader. Ideally we
      // should spit a warning and avoid the situation from happening, but
      // currently there are some defaults uniforms being set and a warning will
      // spam the logs.
      return;
    }
  }

  switch (desc.type) {
    case ShaderDataType_Float1:
      GL_CALL(glUniform1fv(binding, desc.count, uniform.GetData<float>()));
      break;
    case ShaderDataType_Float2:
      GL_CALL(glUniform2fv(binding, desc.count, uniform.GetData<float>()));
      break;
    case ShaderDataType_Float3:
      GL_CALL(glUniform3fv(binding, desc.count, uniform.GetData<float>()));
      break;
    case ShaderDataType_Float4:
      GL_CALL(glUniform4fv(binding, desc.count, uniform.GetData<float>()));
      break;

    case ShaderDataType_Int1:
      GL_CALL(glUniform1iv(binding, desc.count, uniform.GetData<int>()));
      break;
    case ShaderDataType_Int2:
      GL_CALL(glUniform2iv(binding, desc.count, uniform.GetData<int>()));
      break;
    case ShaderDataType_Int3:
      GL_CALL(glUniform3iv(binding, desc.count, uniform.GetData<int>()));
      break;
    case ShaderDataType_Int4:
      GL_CALL(glUniform4iv(binding, desc.count, uniform.GetData<int>()));
      break;

    case ShaderDataType_Float4x4:
      GL_CALL(glUniformMatrix4fv(binding, desc.count, false,
                                 uniform.GetData<float>()));
      break;
    case ShaderDataType_Float3x3:
      GL_CALL(glUniformMatrix3fv(binding, desc.count, false,
                                 uniform.GetData<float>()));
      break;
    case ShaderDataType_Float2x2:
      GL_CALL(glUniformMatrix2fv(binding, desc.count, false,
                                 uniform.GetData<float>()));
      break;
    default:
      LOG(DFATAL) << "Uniform named \"" << desc.name
                  << "\" is set to unsupported type: " << desc.type;
  }
}

const Shader::ShaderImplPtr& Shader::Impl() const { return impl_; }

}  // namespace lull
