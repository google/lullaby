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

#include "lullaby/systems/render/next/shader.h"
#include <vector>

#include "lullaby/generated/flatbuffers/shader_def_generated.h"
#include "lullaby/systems/render/next/detail/glplatform.h"
#include "lullaby/systems/render/next/next_renderer.h"
#include "lullaby/systems/render/next/texture.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/logging.h"
#include "lullaby/generated/material_def_generated.h"

namespace lull {
namespace {
size_t CalculateUniformFieldSize(const ShaderUniformDefT& field) {
  return detail::UniformData::ShaderDataTypeToBytesSize(field.type) *
         std::max(static_cast<size_t>(field.array_size), size_t(1));
}

std::vector<uint8_t> BuildDefaultDataBuffer(const ShaderUniformDefT& parent) {
  std::vector<uint8_t> data;

  // Find the size of the buffer in bytes, including all of its fields.
  size_t buffer_size = 0;
  bool has_defaults = false;
  for (const auto& field : parent.fields) {
    buffer_size += CalculateUniformFieldSize(field);

    if (!field.values.empty() || !field.values_int.empty()) {
      has_defaults = true;
    }
  }

  if (has_defaults) {
    // Build the data buffer.
    data.resize(buffer_size, 0);

    uint8_t* ptr = data.data();
    for (const auto& field : parent.fields) {
      // Copy the data available for each field.
      const size_t field_size = CalculateUniformFieldSize(field);
      if (!field.values.empty()) {
        memcpy(ptr, field.values.data(), field.values.size() * sizeof(float));
      } else if (!field.values_int.empty()) {
        memcpy(ptr, field.values_int.data(),
               field.values_int.size() * sizeof(int));
      }
      ptr += field_size;
    }
  }

  return data;
}
}  // namespace

void Shader::Init(ProgramHnd program, ShaderHnd vs, ShaderHnd fs) {
  if (!program || !vs || !fs) {
    LOG(DFATAL) << "Initializing shader with invalid objects.";
    return;
  }

  program_ = program;
  vs_ = vs;
  fs_ = fs;

  GL_CALL(glUseProgram(*program));

  // Get locations for all the uniforms in the program.
  GLint num_uniforms = 0;
  GL_CALL(glGetProgramiv(*program, GL_ACTIVE_UNIFORMS, &num_uniforms));
  for (GLint i = 0; i < num_uniforms; ++i) {
    GLchar name[512];
    GLsizei length = 0;
    GLint size = 0;
    GLenum type = 0;
    GL_CALL(glGetActiveUniform(*program, i, sizeof(name), &length, &size, &type,
                               name));
    auto iter = strchr(name, '[');
    if (iter) {
      *iter = 0;
    }
    const UniformHnd location = glGetUniformLocation(*program, name);
    uniforms_[Hash(name)] = location;
  }

  if (NextRenderer::SupportsUniformBufferObjects()) {
    GLint num_uniform_blocks = 0;
    glGetProgramiv(*program, GL_ACTIVE_UNIFORM_BLOCKS, &num_uniform_blocks);
    for (GLint i = 0; i < num_uniform_blocks; ++i) {
      GLint length;
      GLchar name[512];
      GL_CALL(glGetActiveUniformBlockiv(*program, i,
                                        GL_UNIFORM_BLOCK_NAME_LENGTH, &length));
      GL_CALL(glGetActiveUniformBlockName(*program, i, sizeof(name), &length,
                                          name));
      name[length] = 0;

      UniformHnd handle = static_cast<int>(uniform_blocks_.size());
      const GLuint index = glGetUniformBlockIndex(*program, name);
      GL_CALL(glUniformBlockBinding(*program_, index, *handle));
      uniform_blocks_[Hash(name)] = handle;
    }
  }

  // Create a mapping from texture usage to texture unit index and uniform.
  for (size_t unit = 0; unit < description_.samplers.size(); ++unit) {
    const ShaderSamplerDefT& sampler = description_.samplers[unit];
    const TextureUsageInfo info = TextureUsageInfo(sampler);
    samplers_[info].unit = static_cast<int>(unit);
    samplers_[info].uniform = uniforms_[Hash(sampler.name)];
  }

  if (description_.samplers.empty()) {
    // Preserve legacy sampler behavior.  In legacy mode, assume that the shader
    // supports all usages, mapping their integer value to the corresponding
    // texture unit index.
    for (int i = MaterialTextureUsage_MIN; i <= MaterialTextureUsage_MAX; ++i) {
      samplers_[TextureUsageInfo(i)].unit = i;

      // For samplers with texture_unit_##### naming, automatically set uniform.
      char uniform_name[] = "texture_unit_#####";
      snprintf(uniform_name, sizeof(uniform_name), "texture_unit_%d", i);

      auto iter = uniforms_.find(Hash(uniform_name));
      if (iter != uniforms_.end()) {
        samplers_[TextureUsageInfo(i)].uniform = iter->second;
      }
    }
  }
}

Shader::~Shader() {
  if (fs_) {
    GL_CALL(glDeleteShader(*fs_));
  }
  if (vs_) {
    GL_CALL(glDeleteShader(*vs_));
  }
  if (program_) {
    GL_CALL(glDeleteProgram(*program_));
  }
  for (const auto& iter : default_ubos_) {
    if (iter.second.Valid()) {
      GLuint ubo = *iter.second;
      GL_CALL(glDeleteBuffers(1, &ubo));
    }
  }
}

bool Shader::IsUniformBlock(HashValue name) const {
  return uniform_blocks_.find(name) != uniform_blocks_.end();
}

UniformHnd Shader::FindUniform(HashValue hash) const {
  auto iter = uniforms_.find(hash);
  return iter != uniforms_.end() ? iter->second : UniformHnd();
}

UniformHnd Shader::FindUniformBlock(HashValue hash) const {
  auto iter = uniform_blocks_.find(hash);
  return iter != uniform_blocks_.end() ? iter->second : UniformHnd();
}

void Shader::Bind() {
  if (program_) {
    GL_CALL(glUseProgram(*program_));
  }
}

bool Shader::SetUniform(HashValue name, const int* data, size_t len,
                        int count) {
  UniformHnd id = FindUniform(name);
  if (program_ && id) {
    switch (len) {
      case 1:
        GL_CALL(glUniform1iv(*id, count, data));
        return true;
      case 2:
        GL_CALL(glUniform2iv(*id, count, data));
        return true;
      case 3:
        GL_CALL(glUniform3iv(*id, count, data));
        return true;
      case 4:
        GL_CALL(glUniform4iv(*id, count, data));
        return true;
      default:
        LOG(DFATAL) << "Unknown uniform dimension: " << len;
    }
  }
  return false;
}

bool Shader::SetUniform(HashValue name, const float* data, size_t len,
                        int count) {
  UniformHnd id = FindUniform(name);
  if (program_ && id) {
    switch (len) {
      case 1:
        GL_CALL(glUniform1fv(*id, count, data));
        return true;
      case 2:
        GL_CALL(glUniform2fv(*id, count, data));
        return true;
      case 3:
        GL_CALL(glUniform3fv(*id, count, data));
        return true;
      case 4:
        GL_CALL(glUniform4fv(*id, count, data));
        return true;
      case 9:
        GL_CALL(glUniformMatrix3fv(*id, count, false, data));
        return true;
      case 16:
        GL_CALL(glUniformMatrix4fv(*id, count, false, data));
        return true;
      default:
        LOG(DFATAL) << "Unknown uniform dimension: " << len;
    }
  }
  return false;
}

const ShaderDescription& Shader::GetDescription() const { return description_; }

template <typename T>
static const T* GetDataPtr(Span<uint8_t> data) {
  return reinterpret_cast<const T*>(data.data());
}

void Shader::BindUniform(HashValue name, ShaderDataType type,
                         Span<uint8_t> data) {
  UniformHnd hnd = FindUniform(name);
  if (!hnd) {
    return;
  }

  const int count = static_cast<int>(
      data.size() / detail::UniformData::ShaderDataTypeToBytesSize(type));

  switch (type) {
    case ShaderDataType_Float1:
      GL_CALL(glUniform1fv(*hnd, count, GetDataPtr<float>(data)));
      break;
    case ShaderDataType_Float2:
      GL_CALL(glUniform2fv(*hnd, count, GetDataPtr<float>(data)));
      break;
    case ShaderDataType_Float3:
      GL_CALL(glUniform3fv(*hnd, count, GetDataPtr<float>(data)));
      break;
    case ShaderDataType_Float4:
      GL_CALL(glUniform4fv(*hnd, count, GetDataPtr<float>(data)));
      break;

    case ShaderDataType_Int1:
      GL_CALL(glUniform1iv(*hnd, count, GetDataPtr<int>(data)));
      break;
    case ShaderDataType_Int2:
      GL_CALL(glUniform2iv(*hnd, count, GetDataPtr<int>(data)));
      break;
    case ShaderDataType_Int3:
      GL_CALL(glUniform3iv(*hnd, count, GetDataPtr<int>(data)));
      break;
    case ShaderDataType_Int4:
      GL_CALL(glUniform4iv(*hnd, count, GetDataPtr<int>(data)));
      break;

    case ShaderDataType_Float4x4:
      GL_CALL(glUniformMatrix4fv(*hnd, count, false, GetDataPtr<float>(data)));
      break;
    case ShaderDataType_Float3x3:
      GL_CALL(glUniformMatrix3fv(*hnd, count, false, GetDataPtr<float>(data)));
      break;
    case ShaderDataType_Float2x2:
      GL_CALL(glUniformMatrix2fv(*hnd, count, false, GetDataPtr<float>(data)));
      break;
    default:
      LOG(DFATAL) << "Unsupported type: " << type;
  }
}

void Shader::BindUniformBlock(HashValue name, UniformBufferHnd ubo) {
  if (!ubo) {
    return;
  }
  UniformHnd hnd = FindUniformBlock(name);
  if (!hnd) {
    return;
  }
  GL_CALL(glBindBufferBase(GL_UNIFORM_BUFFER, static_cast<GLuint>(*hnd), *ubo));
}

void Shader::BindSampler(TextureUsageInfo usage, const TexturePtr& texture) {
  auto iter = samplers_.find(usage);
  if (iter == samplers_.end()) {
    return;
  }

  const Sampler sampler = iter->second;
  if (sampler.unit >= NextRenderer::MaxTextureUnits()) {
    if (texture) {
      LOG(ERROR) << "Invalid unit for texture: " << texture->GetName();
    }
    return;
  }

  UniformHnd uniform_hnd = sampler.uniform;
  TextureHnd texture_hnd = texture ? texture->GetResourceId() : TextureHnd();
  const int target = texture ? texture->GetTarget() : GL_TEXTURE_2D;
  BindTexture(uniform_hnd, texture_hnd, target, sampler.unit);
}

void Shader::BindShaderUniformDef(const ShaderUniformDefT& uniform) {
  const HashValue name = Hash(uniform.name);
  if (uniform.type == ShaderDataType_BufferObject) {
    BindUniformBlock(name, GetDefaultUbo(uniform));
  } else if (!uniform.values.empty()) {
    BindUniform(name, uniform.type, ToByteSpan(uniform.values));
  } else if (!uniform.values_int.empty()) {
    BindUniform(name, uniform.type, ToByteSpan(uniform.values_int));
  }
}

void Shader::BindShaderSamplerDef(const ShaderSamplerDefT& sampler) {
  const TextureUsageInfo usage = TextureUsageInfo(sampler);
  auto iter = samplers_.find(usage);
  if (iter != samplers_.end()) {
    const Sampler& sampler = iter->second;
    BindTexture(sampler.uniform, TextureHnd(), GL_TEXTURE_2D, sampler.unit);
  }
}

void Shader::BindTexture(UniformHnd uniform, TextureHnd texture, int type,
                         int unit) {
  GL_CALL(glActiveTexture(GL_TEXTURE0 + unit));
  if (texture) {
    GL_CALL(glBindTexture(type, *texture));
  }
  if (uniform) {
    GL_CALL(glUniform1i(*uniform, unit));
  }
}

UniformBufferHnd Shader::GetDefaultUbo(const ShaderUniformDefT& uniform) {
  const HashValue name = Hash(uniform.name);
  auto iter = default_ubos_.find(name);
  if (iter != default_ubos_.end()) {
    return iter->second;
  }

  UniformBufferHnd hnd;
  std::vector<uint8_t> data = BuildDefaultDataBuffer(uniform);
  if (!data.empty()) {
    GLuint ubo = 0;
    GL_CALL(glGenBuffers(1, &ubo));
    GL_CALL(glBindBuffer(GL_UNIFORM_BUFFER, ubo));
    GL_CALL(glBufferData(GL_UNIFORM_BUFFER, data.size(), data.data(),
                         GL_STATIC_DRAW));
    hnd = ubo;
  }
  default_ubos_[name] = hnd;
  return hnd;
}

}  // namespace lull
