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

#include "lullaby/systems/render/next/render_factory.h"

namespace lull {

RenderFactory::RenderFactory(Registry* registry, fplbase::Renderer* renderer) {
  fpl_asset_manager_ = std::make_shared<fplbase::AssetManager>(*renderer);
  mesh_factory_ =
      registry->Create<MeshFactory>(registry, renderer, fpl_asset_manager_);
  shader_factory_ = registry->Create<ShaderFactory>(registry, renderer);
  texture_factory_ = registry->Create<TextureFactory>(registry, renderer);
  texture_atlas_factory_ =
      registry->Create<TextureAtlasFactory>(registry, renderer);
  fpl_asset_manager_->StartLoadingTextures();
}

MeshPtr RenderFactory::LoadMesh(const std::string& filename) {
  return mesh_factory_->LoadMesh(filename);
}

ShaderPtr RenderFactory::LoadShader(const std::string& filename) {
  return shader_factory_->LoadShader(filename);
}

bool RenderFactory::IsTextureValid(const TexturePtr& texture) const {
  return (texture && fplbase::ValidTextureHandle(texture->GetResourceId()));
}

TexturePtr RenderFactory::LoadTexture(const std::string& filename,
                                      bool create_mips) {
  return texture_factory_->LoadTexture(filename, create_mips);
}

void RenderFactory::LoadTextureAtlas(const std::string& filename,
                                     bool create_mips) {
  texture_atlas_factory_->LoadTextureAtlas(filename, create_mips);
}

TexturePtr RenderFactory::CreateTextureFromMemory(const void* data,
                                                  const mathfu::vec2i size,
                                                  fplbase::TextureFormat format,
                                                  bool create_mips) {
  return texture_factory_->CreateTextureFromMemory(data, size, format,
                                                   create_mips);
}


TexturePtr RenderFactory::CreateProcessedTexture(
    const TexturePtr& texture, bool create_mips,
    const RenderSystem::TextureProcessor& processor,
    const mathfu::vec2i& output_dimensions) {
  return texture_factory_->CreateProcessedTexture(texture, create_mips,
                                                  processor, output_dimensions);
}

TexturePtr RenderFactory::CreateProcessedTexture(
    const TexturePtr& source_texture, bool create_mips,
    const RenderSystem::TextureProcessor& processor) {
  return texture_factory_->CreateProcessedTexture(source_texture, create_mips,
                                                  processor);
}

TexturePtr RenderFactory::CreateTexture(uint32_t texture_target,
                                        uint32_t texture_id) {
  return texture_factory_->CreateTexture(texture_target, texture_id);
}

void RenderFactory::UpdateAssetLoad() { fpl_asset_manager_->TryFinalize(); }

void RenderFactory::WaitForAssetsToLoad() {
  while (!fpl_asset_manager_->TryFinalize()) {
  }
}

void RenderFactory::CacheTexture(HashValue name, const TexturePtr& texture) {
  texture_factory_->CacheTexture(name, texture);
}

TexturePtr RenderFactory::GetCachedTexture(HashValue texture_hash) {
  return texture_factory_->GetCachedTexture(texture_hash);
}

MeshPtr RenderFactory::CreateMesh(const MeshData& mesh) {
  return mesh_factory_->CreateMesh(mesh);
}

MeshPtr RenderFactory::CreateMesh(HashValue key, const MeshData& mesh) {
  return mesh_factory_->CreateMesh(key, mesh);
}

void RenderFactory::StartLoadingAssets() {
  fpl_asset_manager_->StartLoadingTextures();
}

void RenderFactory::StopLoadingAssets() {
  fpl_asset_manager_->StopLoadingTextures();
}

const TexturePtr& RenderFactory::GetWhiteTexture() const {
  return texture_factory_->GetWhiteTexture();
}

const TexturePtr& RenderFactory::GetInvalidTexture() const {
  return texture_factory_->GetInvalidTexture();
}

}  // namespace lull
