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
#include "lullaby/util/flatbuffer_reader.h"
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

ShaderUniformDefT CreateUniformDef(const char* name, ShaderDataType type,
                                   unsigned int array_size) {
  ShaderUniformDefT uniform_def;
  uniform_def.name = name;
  uniform_def.type = type;
  uniform_def.array_size = array_size;

  return uniform_def;
}

HashValue HashLoadParams(const ShaderCreateParams& params) {
  HashValue hash = Hash(params.shading_model);
  for (const HashValue it : params.environment) {
    hash ^= it;
  }
  for (const auto& it : params.features) {
    hash ^= it;
  }
  return hash;
}
}  // namespace

ShaderFactory::ShaderFactory(Registry* registry) : registry_(registry) {}

ShaderPtr ShaderFactory::LoadShader(const ShaderCreateParams& params) {
  const HashValue key = HashLoadParams(params);

  ShaderPtr shader =
      shaders_.Create(key, [this, &params]() { return LoadImpl(params); });
  shaders_.Release(key);
  return shader;
}

ShaderPtr ShaderFactory::LoadImpl(const ShaderCreateParams& params) {
  ShaderPtr shader =
      LoadLullShaderImpl(params.shading_model + ".lullshader", params);
  if (shader) {
    return shader;
  }
  return LoadFplShaderImpl(params.shading_model + ".fplshader");
}

ShaderPtr ShaderFactory::LoadShaderFromDef(const ShaderDefT& shader_def,
                                           const ShaderCreateParams& params) {
  // Pass the shader def through the assembler.
  const ShaderData shader_data(shader_def, params);
  if (!shader_data.IsValid()) {
    LOG(DFATAL) << "Failed to proces shader.";
    return nullptr;
  }

  // Construct the shader handles.
  std::array<ShaderHnd, ShaderData::kNumStages> shader_handles;
  for (int i = static_cast<int>(ShaderStageType_MIN);
       i <= static_cast<int>(ShaderStageType_MAX); ++i) {
    const ShaderStageType shader_stage = static_cast<ShaderStageType>(i);
    if (!shader_data.HasStage(shader_stage)) {
      continue;
    }

    // Compile the shader stage.
    shader_handles[shader_stage] = CompileShader(
        shader_data.GetStageCode(shader_stage).c_str(), shader_stage);
    if (!shader_handles[shader_stage]) {
      LOG(DFATAL) << "Failed to compile shader stage " << shader_stage;
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
  ShaderPtr shader = std::make_shared<Shader>(shader_data.GetDescription());
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

ShaderPtr ShaderFactory::LoadLullShaderImpl(const std::string& filename,
                                            const ShaderCreateParams& params) {
  auto* asset_loader = registry_->Get<AssetLoader>();
  auto asset = asset_loader->LoadNow<SimpleAsset>(filename);
  if (!asset || asset->GetSize() == 0) {
    return nullptr;
  }
  const ShaderDef* shader_flatbuffer =
      flatbuffers::GetRoot<ShaderDef>(asset->GetData());
  if (!shader_flatbuffer) {
    return nullptr;
  }

  // TODO(b/73493108): Read shader from flatbuffer table instead of ShaderDefT.
#ifdef SHADER_DEBUG
  LOG(INFO) << "Building shader: " << filename << ".";
#endif
  ShaderDefT shader_def;
  ReadFlatbuffer(&shader_def, shader_flatbuffer);
  return LoadShaderFromDef(shader_def, params);
}

ShaderPtr ShaderFactory::LoadFplShaderImpl(const std::string& filename) {
  auto* asset_loader = registry_->Get<AssetLoader>();
  auto asset = asset_loader->LoadNow<SimpleAsset>(filename);
  const shaderdef::Shader* def = shaderdef::GetShader(asset->GetData());
  if (def == nullptr) {
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

  const std::string safe_source =
      SanitizeShaderSource(source, GetShaderProfile());

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

ProgramHnd ShaderFactory::LinkProgram(ShaderHnd vs, ShaderHnd fs,
                                      Span<ShaderAttributeDefT> attributes) {
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
