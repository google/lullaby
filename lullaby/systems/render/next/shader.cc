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

static constexpr int kMaxTexturesPerShader = 8;

void Shader::Init(ProgramHnd program, ShaderHnd vs, ShaderHnd fs) {
  if (!program || !vs || !fs) {
    LOG(DFATAL) << "Initializing shader with invalid objects.";
    return;
  }

  program_ = program;
  vs_ = vs;
  fs_ = fs;

  // Look up variables that are builtin, but still optionally present in a
  // shader.
  GL_CALL(glUseProgram(*program));

  // Set up the uniforms the shader uses for texture access.
  char texture_unit_name[] = "texture_unit_#####";
  for (int i = 0; i < kMaxTexturesPerShader; i++) {
    snprintf(texture_unit_name, sizeof(texture_unit_name), "texture_unit_%d",
             i);
    auto loc = glGetUniformLocation(*program, texture_unit_name);
    if (loc >= 0) {
      GL_CALL(glUniform1i(loc, i));
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

void Shader::SetUniformsDefs(Span<ShaderUniformDefT> uniform_defs) {
  if (!program_) {
    LOG(ERROR) << "Defining uniforms on an invalid shader.";
    return;
  }

  GL_CALL(glUseProgram(*program_));
  for (const auto& uniform_def : uniform_defs) {
    // Find the uniform handle and save it.
    UniformHnd handle =
        glGetUniformLocation(*program_, uniform_def.name.c_str());
    uniforms_[Hash(uniform_def.name)] = handle;
  }
}

void Shader::Bind() {
  if (program_) {
    GL_CALL(glUseProgram(*program_));
  }
}

void Shader::SetUniform(UniformHnd id, const float* value, size_t len,
                        int count) {
  if (program_ && id) {
    switch (len) {
      case 1:
        DCHECK_EQ(count, 1);
        GL_CALL(glUniform1f(*id, *value));
        break;
      case 2:
        GL_CALL(glUniform2fv(*id, count, value));
        break;
      case 3:
        GL_CALL(glUniform3fv(*id, count, value));
        break;
      case 4:
        GL_CALL(glUniform4fv(*id, count, value));
        break;
      case 16:
        GL_CALL(glUniformMatrix4fv(*id, count, false, value));
        break;
      default:
        LOG(DFATAL) << "Unknown uniform dimension: " << len;
        break;
    }
  }
}

const Shader::Description& Shader::GetDescription() const {
  return description_;
}

}  // namespace lull
