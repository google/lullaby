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

#include "lullaby/systems/render/next/texture_atlas_factory.h"

#include "fplbase/texture_atlas_generated.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/systems/render/next/texture_factory.h"

namespace lull {

TextureAtlasFactory::TextureAtlasFactory(Registry* registry,
                                         fplbase::Renderer* renderer)
    : registry_(registry), fpl_renderer_(renderer) {}

void TextureAtlasFactory::LoadTextureAtlas(const std::string& filename,
                                           bool create_mips) {
  const HashValue key = Hash(filename.c_str());
  atlases_.Create(key, [&]() {
    auto* asset_loader = registry_->Get<AssetLoader>();
    auto asset = asset_loader->LoadNow<SimpleAsset>(filename);
    if (asset->GetSize() == 0) {
      return std::shared_ptr<TextureAtlas>();
    }

    auto atlasdef = atlasdef::GetTextureAtlas(asset->GetData());
    auto texture_factory = registry_->Get<TextureFactory>();

    TexturePtr texture = texture_factory->LoadTexture(
        atlasdef->texture_filename()->c_str(), create_mips);

    std::vector<std::string> subtextures;
    subtextures.reserve(atlasdef->entries()->Length());
    for (const auto* entry : *atlasdef->entries()) {
      const char* name = entry->name()->c_str();
      subtextures.emplace_back(name);

      const mathfu::vec2 size(entry->size()->x(), entry->size()->y());
      const mathfu::vec2 location(entry->location()->x(),
                                  entry->location()->y());
      const mathfu::vec4 uv_bounds(location.x, location.y, size.x, size.y);
      texture_factory->CreateSubtexture(Hash(name), texture, uv_bounds);
    }

    auto atlas = std::make_shared<TextureAtlas>();
    atlas->Init(texture, std::move(subtextures));
    return atlas;
  });
}

void TextureAtlasFactory::ReleaseTextureAtlasFromCache(HashValue key) {
  std::shared_ptr<TextureAtlas> atlas = atlases_.Find(key);
  if (atlas == nullptr) {
    return;
  }

  auto texture_factory = registry_->Get<TextureFactory>();
  for (const auto& name : atlas->Subtextures()) {
    const HashValue key = Hash(name.c_str());
    texture_factory->ReleaseTextureFromCache(key);
  }
  atlases_.Release(key);
}

}  // namespace lull
