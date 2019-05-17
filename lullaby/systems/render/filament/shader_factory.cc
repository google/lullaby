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

#include "lullaby/systems/render/filament/shader_factory.h"

#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/systems/render/filament/shader_material_builder.h"
#include "lullaby/util/filename.h"
#include "lullaby/util/flatbuffer_reader.h"

namespace lull {

// Creates a unique hash given the shading model and selection parameters.
static HashValue CreateShaderHash(string_view shading_model,
                                  const ShaderSelectionParams& params) {
  HashValue hash = Hash(shading_model);
  for (HashValue feature : params.environment) {
    hash = HashCombine(hash, feature);
  }
  for (HashValue feature : params.features) {
    hash = HashCombine(hash, feature);
  }
  return hash;
}

ShaderFactory::ShaderFactory(Registry* registry, filament::Engine* engine)
    : registry_(registry), engine_(engine), shading_model_path_("shaders/") {
  // filamat::MaterialBuilder requires calls to its static init and shutdown.
  filamat::MaterialBuilder::init();
}

ShaderFactory::~ShaderFactory() {
  // filamat::MaterialBuilder requires calls to its static init and shutdown.
  filamat::MaterialBuilder::shutdown();
}

void ShaderFactory::SetShadingModelPath(string_view path) {
  shading_model_path_ = std::string(path);
}

ShaderPtr ShaderFactory::CreateShader(string_view shader_name,
                                      const ShaderSelectionParams& params) {
  // Find the asset with the given shader name.
  ShaderAssetPtr asset = FindShaderAsset(shader_name);
  DCHECK(asset != nullptr);

  // Create a unique key for the shader instance using the selection parameters.
  HashValue key = 0;
  if (asset->model.empty()) {
    key = CreateShaderHash(shader_name, params);
  } else {
    key = CreateShaderHash(asset->model, params);
  }

  // Find and return a cached Shader object that has already been created from
  // the asset associated with the shader_name.
  ShaderPtr shader = shaders_.Find(key);
  if (shader) {
    return shader;
  }

  if (asset->type == kFilamentMatc) {
    // Build the shader using the matc binary data.
    ShaderMaterialBuilder builder(engine_, asset->model, asset->GetData(),
                                  asset->GetSize());
    return BuildShader(key, &builder);
  } else if (asset->type == kLullShader) {
    // Build the shader from the lullshader.
    ShaderDefT def;
    ReadFlatbuffer(&def, flatbuffers::GetRoot<ShaderDef>(asset->GetData()));
    ShaderMaterialBuilder builder(engine_, asset->model, &def, params);
    return BuildShader(key, &builder);
  } else {
    // Build the shader programatically.
    ShaderMaterialBuilder builder(engine_, std::string(shader_name), nullptr,
                                  params);
    return BuildShader(key, &builder);
  }
}

ShaderPtr ShaderFactory::BuildShader(HashValue key,
                                     ShaderMaterialBuilder* builder) {
  if (!builder->IsValid()) {
    return nullptr;
  }

  // Build the filament Material using the ShaderMaterialBuilder and assign
  filament::Engine* engine = engine_;
  filament::Material* material = builder->GetFilamentMaterial();
  Shader::FMaterialPtr ptr(
      material, [engine](filament::Material* obj) { engine->destroy(obj); });

  // Create the shader object using the Filament Material.
  auto shader = std::make_shared<Shader>();
  shader->Init(std::move(ptr), builder->GetDescription());
  if (key) {
    shaders_.Register(key, shader);
    shaders_.Release(key);
  }
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

ShaderFactory::ShaderAssetPtr ShaderFactory::FindShaderAsset(
    string_view shader_name) {
  // Extract the shading model name from the shader_name.  If the shader_name is
  // a shading model name, then this should do nothing.
  std::string model(shader_name);
  model = RemoveDirectoryAndExtensionFromFilename(model);
  std::transform(model.begin(), model.end(), model.begin(), ::tolower);

  // First attempt to load the assets directly if the name ends with a known
  // extension (eg. .matc or .lullshader).
  ShaderAssetPtr asset = nullptr;
  if (EndsWith(shader_name, ".matc")) {
    asset = LoadShaderAsset(shader_name, model, kFilamentMatc);
  } else if (EndsWith(shader_name, ".lullshader")) {
    asset = LoadShaderAsset(shader_name, model, kLullShader);
  }

  // If unsuccessul, attempt to load the shader as first a matc file then a
  // lullmodel file.
  if (asset == nullptr || asset->GetSize() == 0) {
    const std::string basename = JoinPath(shading_model_path_, model);
    asset = LoadShaderAsset(basename + ".matc", model, kFilamentMatc);
    if (asset == nullptr || asset->GetSize() == 0) {
      asset = LoadShaderAsset(basename + ".lullshader", model, kLullShader);
    }
  }
  return asset;
}

ShaderFactory::ShaderAssetPtr ShaderFactory::LoadShaderAsset(
    string_view filename, string_view model, AssetType type) {
  return assets_.Create(Hash(filename), [&]() {
    auto* asset_loader = registry_->Get<AssetLoader>();
    auto asset = asset_loader->LoadNow<ShaderAsset>(std::string(filename));
    if (asset && asset->GetSize() > 0) {
      asset->type = type;
      asset->model = std::string(model);
    }
    return asset;
  });
}
}  // namespace lull
