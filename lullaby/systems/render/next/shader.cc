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

#include "lullaby/systems/render/next/detail/glplatform.h"
#include "lullaby/util/logging.h"

namespace lull {

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

  // Create a mapping from texture usage to texture unit index and uniform.
  for (size_t unit = 0; unit < description_.samplers.size(); ++unit) {
    const ShaderSamplerDefT& sampler = description_.samplers[unit];
    samplers_[sampler.usage].texture_unit = static_cast<int>(unit);
    samplers_[sampler.usage].uniform = uniforms_[Hash(sampler.name)];
  }

  if (description_.samplers.empty()) {
    // Preserve legacy sampler behavior.  In legacy mode, assume that the shader
    // supports all usages, mapping their integer value to the corresponding
    // texture unit index.
    for (int i = MaterialTextureUsage_MIN; i <= MaterialTextureUsage_MAX; ++i) {
      samplers_[i].texture_unit = i;

      // For samplers with texture_unit_##### naming, automatically set uniform.
      char uniform_name[] = "texture_unit_#####";
      snprintf(uniform_name, sizeof(uniform_name), "texture_unit_%d", i);

      auto iter = uniforms_.find(Hash(uniform_name));
      if (iter != uniforms_.end()) {
        samplers_[i].uniform = iter->second;
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
}

UniformHnd Shader::FindUniform(const char* name) {
  const HashValue hashed_name = Hash(name);
  UniformHnd handle = FindUniform(hashed_name);
  if (handle) {
    return handle;
  }

  if (program_) {
    GL_CALL(glUseProgram(*program_));
    handle = glGetUniformLocation(*program_, name);
    uniforms_[hashed_name] = handle;
    return handle;
  } else {
    return UniformHnd();
  }
}

UniformHnd Shader::FindUniform(HashValue hash) const {
  const auto iter = uniforms_.find(hash);
  if (iter == uniforms_.end()) {
    return UniformHnd();
  } else {
    return iter->second;
  }
}

void Shader::Bind() {
  if (program_) {
    GL_CALL(glUseProgram(*program_));
  }
}

bool Shader::SetUniform(HashValue name, const int* data, size_t len,
                        int count) {
  return SetUniform(FindUniform(name), data, len, count);
}

bool Shader::SetUniform(HashValue name, const float* data, size_t len,
                        int count) {
  return SetUniform(FindUniform(name), data, len, count);
}

bool Shader::SetUniform(UniformHnd id, const int* data, size_t len,
                        int count) {
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

bool Shader::SetUniform(UniformHnd id, const float* data, size_t len,
                        int count) {
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

const Shader::Description& Shader::GetDescription() const {
  return description_;
}

Shader::Sampler Shader::GetSampler(MaterialTextureUsage usage) const {
  return samplers_[usage];
}

void Shader::BindUniform(UniformHnd hnd, const UniformData& uniform) {
  const uint8_t* data = uniform.GetData<uint8_t>();
  const size_t size = uniform.Size();
  BindUniform(hnd, uniform.Type(), {data, size});
}

template <typename T>
static const T* GetDataPtr(Span<uint8_t> data) {
  return reinterpret_cast<const T*>(data.data());
}

void Shader::BindUniform(UniformHnd hnd, ShaderDataType type,
                         Span<uint8_t> data) {
  if (!hnd) {
    return;
  }

  const int count = static_cast<int>(
      data.size() / UniformData::ShaderDataTypeToBytesSize(type));

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

}  // namespace lull
