/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#include "redux/engines/render/shader_factory.h"

#include <string>

#include "redux/data/asset_defs/shader_asset_def_generated.h"
#include "redux/engines/render/filament/filament_render_engine.h"
#include "redux/engines/render/filament/filament_shader.h"
#include "redux/engines/render/shader.h"
#include "redux/modules/base/asset_loader.h"

namespace redux {

class ShaderFactory::ShaderAsset {
 public:
  ShaderPtr shader;
};

ShaderFactory::ShaderFactory(Registry* registry) : registry_(registry) {}

ShaderPtr ShaderFactory::CreateShader(std::string_view shading_model) {
  if (shading_model.empty()) {
    auto shader = std::make_shared<FilamentShader>(registry_, nullptr);
    return std::static_pointer_cast<Shader>(shader);
  }

  const std::string uri = std::string(shading_model) + ".rxshader";
  const HashValue key = Hash(uri);

  auto shader_asset = assets_.Find(key);
  if (shader_asset == nullptr) {
    auto asset_loader = registry_->Get<AssetLoader>();
    auto asset = asset_loader->LoadNow(uri);
    CHECK(asset.ok());

    const auto* def = flatbuffers::GetRoot<ShaderAssetDef>(asset->GetBytes());
    auto shader = std::make_shared<FilamentShader>(registry_, def);

    shader_asset = std::make_shared<ShaderAsset>();
    shader_asset->shader = std::static_pointer_cast<Shader>(shader);
    assets_.Register(key, shader_asset);
  }
  return shader_asset->shader;
}

}  // namespace redux
