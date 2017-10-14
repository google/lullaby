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

}  // namespace

ShaderFactory::ShaderFactory(Registry* registry, fplbase::Renderer* renderer)
    : registry_(registry), fpl_renderer_(renderer) {}

ShaderPtr ShaderFactory::LoadShader(const std::string& filename) {
  const HashValue key = Hash(filename.c_str());
  ShaderPtr shader = shaders_.Create(key, [&]() {
    ShaderPtr shader = nullptr;
    auto impl = LoadImpl(filename);
    if (impl) {
      shader = std::make_shared<Shader>();
      shader->Init(fpl_renderer_, std::move(impl));
    }
    return shader;
  });
  shaders_.Release(key);
  return shader;
}

std::unique_ptr<fplbase::Shader> ShaderFactory::LoadImpl(
    const std::string& filename) {
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

  fplbase::Shader* shader = fpl_renderer_->CompileAndLinkShader(
      def->vertex_shader()->c_str(), def->fragment_shader()->c_str());
  if (shader == nullptr) {
    LOG(ERROR) << "Failed to compile/link shader.";
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
    LOG(ERROR) << "GL Error:-------------------------------";
    LOG(ERROR) << fpl_renderer_->last_error();
    shader = fpl_renderer_->CompileAndLinkShader(kFallbackVS, kFallbackFS);
  }
  return std::unique_ptr<fplbase::Shader>(shader);
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

}  // namespace lull
