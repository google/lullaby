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

#include "lullaby/systems/render/next/shader_factory.h"

#include "fplbase/shader_generated.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/systems/render/next/detail/glplatform.h"
#include "lullaby/systems/render/next/gl_helpers.h"
#include "lullaby/systems/render/next/mesh.h"
#include "lullaby/util/logging.h"

namespace lull {
namespace {
constexpr const char kFallbackVS[] =
    "attribute vec4 aPosition;\n"
    "uniform mat4 model_view_projection;\n"
    "void main() {\n"
    "  gl_Position = model_view_projection * aPosition;\n"
    "}";

constexpr const char kFallbackFS[] =
    "uniform lowp vec4 color;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(color.rgb * color.a, color.a);\n"
    "}\n";

void ReleaseShadersArray(Span<ShaderHnd> shaders) {
  for (auto& shader : shaders) {
    if (shader) {
      GL_CALL(glDeleteShader(*shader));
    }
  }
}

ShaderUniformDefT CreateUniformDef(const char* name, ShaderUniformType type,
                                   unsigned int array_size) {
  ShaderUniformDefT uniform_def;
  uniform_def.name = name;
  uniform_def.type = type;
  uniform_def.array_size = array_size;

  return uniform_def;
}

const ShaderUniformDefT* FindShaderUniformDefByName(
    const std::vector<ShaderUniformDefT>& uniforms, const std::string& name) {
  for (const auto& it : uniforms) {
    if (it.name == name) {
      return &it;
    }
  }
  return nullptr;
}

const ShaderVertexAttributeDefT* FindShaderVertexAttributeDefByName(
    const std::vector<ShaderVertexAttributeDefT>& attributes,
    const std::string& name) {
  for (const auto& it : attributes) {
    if (it.name == name) {
      return &it;
    }
  }
  return nullptr;
}

bool CompareShaderDataDef(const ShaderDataDefT& lhs,
                          const ShaderDataDefT& rhs) {
  if (lhs.type != rhs.type) {
    return false;
  } else if (lhs.array_size != rhs.array_size) {
    return false;
  } else if (lhs.fields.size() != rhs.fields.size()) {
    return false;
  }

  for (size_t i = 0; i < lhs.fields.size(); ++i) {
    if (!CompareShaderDataDef(lhs.fields[i], rhs.fields[i])) {
      return false;
    }
  }
  return true;
}

bool CompareShaderUniformDefs(const ShaderUniformDefT& lhs,
                              const ShaderUniformDefT& rhs) {
  if (lhs.type != rhs.type) {
    return false;
  } else if (lhs.array_size != rhs.array_size) {
    return false;
  } else if (lhs.fields.size() != rhs.fields.size()) {
    return false;
  }
  for (size_t i = 0; i < lhs.fields.size(); ++i) {
    if (!CompareShaderDataDef(lhs.fields[i], rhs.fields[i])) {
      return false;
    }
  }
  return true;
}

bool CompareShaderVertexAttributeDefs(const ShaderVertexAttributeDefT& lhs,
                                      const ShaderVertexAttributeDefT& rhs) {
  return lhs.type == rhs.type && lhs.usage == rhs.usage;
}

}  // namespace

ShaderFactory::ShaderFactory(Registry* registry) : registry_(registry) {}

ShaderPtr ShaderFactory::LoadShader(const std::string& filename) {
  const HashValue key = Hash(filename.c_str());
  ShaderPtr shader =
      shaders_.Create(key, [this, &filename]() { return LoadImpl(filename); });
  shaders_.Release(key);
  return shader;
}

ShaderPtr ShaderFactory::LoadShaderFromDef(const ShaderDefT& shader_def) {
  // Shader sanitization variable used later inside the for loop.
  const std::string max_uniform_components =
      "MAX_VERTEX_UNIFORM_COMPONENTS " +
      std::to_string(GlMaxVertexUniformComponents());
  string_view defines[] = {
    max_uniform_components
  };

  // Shader information for constructing the shadeers.
  std::array<ShaderHnd, ShaderStageType_MAX + 1> shader_handles;
  Shader::Description shader_description;

  // Link the shader stages.
  for (const auto& stage : shader_def.stages) {
    if (shader_handles[stage.type]) {
      LOG(DFATAL) << "Shader with two stages of the same type.";
      ReleaseShadersArray(shader_handles);
      return nullptr;
    }

    // Assemble the shader stage from its snippets.
    std::string stage_src = "";
    for (const auto& snippet : stage.snippets) {
      // Construct the source code for the stage.
      if (!snippet.code.empty()) {
        stage_src.append(snippet.code);
      }

      // Add the uniforms to the shader description.
      for (const auto& uniform : snippet.uniforms) {
        // Check for an existing uniform with the same name and that they match
        // in definition.
        const ShaderUniformDefT* stored_uniform = FindShaderUniformDefByName(
            shader_description.uniforms, uniform.name);
        if (stored_uniform &&
            !CompareShaderUniformDefs(*stored_uniform, uniform)) {
          LOG(DFATAL) << "Snippets contain uniforms with same name ("
                      << uniform.name << ") but different definitions.";
          ReleaseShadersArray(shader_handles);
          return nullptr;
        }
        shader_description.uniforms.push_back(uniform);
      }

      // Add the vertex attributes to the shader description.
      for (const auto& attribute : snippet.attributes) {
        // Check for an existing attribute with the same name and that they
        // match in definition.
        const ShaderVertexAttributeDefT* stored_attribute =
            FindShaderVertexAttributeDefByName(shader_description.attributes,
                                               attribute.name);
        if (stored_attribute &&
            !CompareShaderVertexAttributeDefs(*stored_attribute, attribute)) {
          LOG(DFATAL) << "Snippets contain vertex attributes with same name ("
                      << attribute.name << ") but different definitions.";
          ReleaseShadersArray(shader_handles);
          return nullptr;
        }
        shader_description.attributes.push_back(attribute);
      }
    }

    // Perform shader sanitization.
    const std::string sanitized_stage_source =
      SanitizeShaderSource(stage_src, GetShaderProfile(), defines);

    // Compile the shader stage.
    shader_handles[stage.type] =
        CompileShader(sanitized_stage_source.c_str(), stage.type);
    if (!shader_handles[stage.type]) {
      LOG(DFATAL) << "Failed to compile shader stage " << stage.type;
      ReleaseShadersArray(shader_handles);
      return nullptr;
    }
  }

  if (!shader_handles[ShaderStageType_Vertex]) {
    LOG(DFATAL) << "Shader must have a vertex stage.";
    ReleaseShadersArray(shader_handles);
    return nullptr;
  }
  if (!shader_handles[ShaderStageType_Fragment]) {
    LOG(DFATAL) << "Shader must have a fragment stage.";
    ReleaseShadersArray(shader_handles);
    return nullptr;
  }

  // Link the shader program.
  ProgramHnd program = LinkProgram(shader_handles[ShaderStageType_Vertex],
                                   shader_handles[ShaderStageType_Fragment]);
  if (!program) {
    // Failed to create shader program! Cleanup and return a nullptr.
    ReleaseShadersArray(shader_handles);
    return nullptr;
  }

  // Initialize and return the shader.
  ShaderPtr shader = std::make_shared<Shader>(std::move(shader_description));
  shader->Init(program, shader_handles[ShaderStageType_Vertex],
               shader_handles[ShaderStageType_Fragment]);
  return shader;
}

ShaderPtr ShaderFactory::GetCachedShader(HashValue key) const {
  return shaders_.Find(key);
}

void ShaderFactory::CacheShader(HashValue key, const ShaderPtr& shader) {
  shaders_.Create(key, [shader] { return shader; });
}

void ShaderFactory::ReleaseShaderFromCache(HashValue key) {
  shaders_.Release(key);
}

ShaderPtr ShaderFactory::LoadImpl(const std::string& filename) {
  auto* asset_loader = registry_->Get<AssetLoader>();
  auto asset = asset_loader->LoadNow<SimpleAsset>(filename);
  const shaderdef::Shader* def = shaderdef::GetShader(asset->GetData());
  if (def == nullptr) {
    LOG(DFATAL) << "Failed to read shaderdef.";
    return nullptr;
  } else if (def->vertex_shader() == nullptr) {
    LOG(DFATAL) << "Failed to read vertex shader code from shaderdef.";
    return nullptr;
  } else if (def->fragment_shader() == nullptr) {
    LOG(DFATAL) << "Failed to read fragment shader code from shaderdef.";
    return nullptr;
  }

  ShaderPtr shader = CompileAndLink(def->vertex_shader()->c_str(),
                                    def->fragment_shader()->c_str());
  if (shader == nullptr) {
    LOG(ERROR) << "Original: ------------------------------";
    if (def->original_sources()) {
      for (unsigned int i = 0; i < def->original_sources()->size(); ++i) {
        const auto& source = def->original_sources()->Get(i);
        LOG(ERROR) << source->c_str();
      }
    }
    LOG(ERROR) << "Vertex: --------------------------------";
    LOG(ERROR) << def->vertex_shader()->c_str();
    LOG(ERROR) << "Fragment: ------------------------------";
    LOG(ERROR) << def->fragment_shader()->c_str();
    LOG(DFATAL) << "Failed to compile/link shader!";
    shader = CompileAndLink(kFallbackVS, kFallbackFS);
  }

  // Declare default uniforms for the shader.
  static const unsigned int kDefaultNumBoneTransformsAsVec4Array = 4 * 256;
  const ShaderUniformDefT uniform_defs[] = {
      CreateUniformDef("model_view_projection", ShaderUniformType_Float4x4, 0),
      CreateUniformDef("model", ShaderUniformType_Float4x4, 0),
      CreateUniformDef("view", ShaderUniformType_Float4x4, 0),
      CreateUniformDef("projection", ShaderUniformType_Float4x4, 0),
      CreateUniformDef("mat_normal", ShaderUniformType_Float3x3, 0),
      CreateUniformDef("camera_pos", ShaderUniformType_Float4, 0),
      CreateUniformDef("camera_dir", ShaderUniformType_Float3, 0),
      CreateUniformDef("color", ShaderUniformType_Float4, 0),
      CreateUniformDef("time", ShaderUniformType_Float1, 0),
      CreateUniformDef("uv_bounds", ShaderUniformType_Float2, 0),
      CreateUniformDef("clamp_bounds", ShaderUniformType_Float2, 0),
      CreateUniformDef("uIsRightEye", ShaderUniformType_Int1, 0),
      CreateUniformDef("light_pos", ShaderUniformType_Float4, 0),
      CreateUniformDef("bone_transforms", ShaderUniformType_Float4,
                       kDefaultNumBoneTransformsAsVec4Array)};

  shader->SetUniformsDefs(uniform_defs);

  return shader;
}

ShaderHnd ShaderFactory::CompileShader(const char* source,
                                       ShaderStageType stage) {
  const GLenum gl_stage =
      stage == ShaderStageType_Vertex ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
  ShaderHnd shader = glCreateShader(gl_stage);
  if (!shader) {
    LOG(DFATAL) << "Could not create shader object.";
    return shader;
  }

  const std::string max_uniform_components =
      "MAX_VERTEX_UNIFORM_COMPONENTS " +
      std::to_string(GlMaxVertexUniformComponents());
  string_view defines[] = {
    max_uniform_components
  };
  const std::string safe_source =
      SanitizeShaderSource(source, GetShaderProfile(), defines);

  const char* safe_source_cstr = safe_source.c_str();
  GL_CALL(glShaderSource(*shader, 1, &safe_source_cstr, nullptr));
  GL_CALL(glCompileShader(*shader));

  GLint success;
  GL_CALL(glGetShaderiv(*shader, GL_COMPILE_STATUS, &success));
  if (success == GL_FALSE) {
    GLint length = 0;
    GL_CALL(glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &length));
    std::string error(length + 1, '\0');
    GL_CALL(glGetShaderInfoLog(*shader, length, &length, &error[0]));
    LOG(ERROR) << "Could not compiler shader!";
    LOG(ERROR) << "Error: \n" << error;
    LOG(ERROR) << "Source: \n" << safe_source;

    GL_CALL(glDeleteShader(*shader));
    return ShaderHnd();
  }
  return shader;
}

ProgramHnd ShaderFactory::LinkProgram(
    ShaderHnd vs, ShaderHnd fs, Span<ShaderVertexAttributeDefT> attributes) {
  if (!vs || !fs) {
    LOG(DFATAL) << "Invalid shaders for program.";
    return ProgramHnd();
  }

  const ProgramHnd program = glCreateProgram();
  if (!program) {
    LOG(DFATAL) << "Could not create program object.";
    return ProgramHnd();
  }

  GL_CALL(glAttachShader(*program, *vs));
  GL_CALL(glAttachShader(*program, *fs));
  if (attributes.empty()) {
    const auto mesh_default_attributes = GetDefaultVertexAttributes();
    for (size_t i = 0; i < mesh_default_attributes.size(); ++i) {
      GL_CALL(glBindAttribLocation(*program, mesh_default_attributes[i].second,
                                   mesh_default_attributes[i].first));
    }
  }
  for (size_t i = 0; i < attributes.size(); ++i) {
    GL_CALL(glBindAttribLocation(*program, static_cast<GLuint>(i),
                                 attributes[i].name.c_str()));
  }
  GL_CALL(glLinkProgram(*program));

  GLint status;
  GL_CALL(glGetProgramiv(*program, GL_LINK_STATUS, &status));
  if (status == GL_FALSE) {
    GLint length = 0;
    GL_CALL(glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &length));
    std::string error(length + 1, '\0');
    GL_CALL(glGetProgramInfoLog(*program, length, &length, &error[0]));
    LOG(ERROR) << "Could not link program!";
    LOG(ERROR) << "Error: \n" << error;

    GL_CALL(glDeleteProgram(*program));
    return ProgramHnd();
  }

  return program;
}

ShaderPtr ShaderFactory::CompileAndLink(const char* vs_source,
                                        const char* fs_source) {
  const ShaderHnd vs = CompileShader(vs_source, ShaderStageType_Vertex);
  const ShaderHnd fs = CompileShader(fs_source, ShaderStageType_Fragment);

  ProgramHnd program;
  if (vs && fs) {
    program = LinkProgram(vs, fs);
  }

  if (program && fs && vs) {
    ShaderPtr shader = std::make_shared<Shader>();
    shader->Init(program, fs, vs);
    return shader;
  }

  if (fs) {
    GL_CALL(glDeleteShader(*fs));
  }
  if (vs) {
    GL_CALL(glDeleteShader(*vs));
  }
  if (program) {
    GL_CALL(glDeleteProgram(*program));
  }
  return nullptr;
}

}  // namespace lull
