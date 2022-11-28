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

#include "redux/engines/render/filament/filament_shader.h"

#include <utility>

#include "redux/engines/render/filament/filament_render_engine.h"
#include "redux/engines/render/filament/filament_utils.h"
#include "redux/modules/base/logging.h"

namespace redux {

template <typename T>
using FlatVector = flatbuffers::Vector<T>;

static absl::btree_set<HashValue> ReadFlags(const FlatVector<uint32_t>* vec) {
  absl::btree_set<HashValue> flags;
  if (vec) {
    for (const auto& flag : *vec) {
      flags.emplace(flag);
    }
  }
  return flags;
}

template <typename T>
absl::Span<const std::byte> AsBytes(const FlatVector<T>& vec) {
  const std::byte* bytes = reinterpret_cast<const std::byte*>(vec.data());
  const size_t size = vec.size() * sizeof(T);
  return {bytes, size};
}

static FilamentResourcePtr<filament::Material> ReadFilamentMaterial(
    filament::Engine* engine, const FlatVector<uint8_t>* matc) {
  CHECK(matc);
  filament::Material::Builder builder;
  builder.package(matc->data(), matc->size());
  return MakeFilamentResource(builder.build(*engine), engine);
}

FilamentShader::FilamentShader(Registry* registry, const ShaderAssetDef* def) {
  fengine_ = GetFilamentEngine(registry);
  if (def && def->variants()) {
    for (const auto& variant : *def->variants()) {
      CHECK(variant);
      BuildVariant(variant);
    }
  }
}

void FilamentShader::BuildVariant(const ShaderVariantAssetDef* def) {
  auto variant = std::make_unique<Variant>();
  variant->fmaterial = ReadFilamentMaterial(fengine_, def->filament_material());
  variant->conditions = ReadFlags(def->conditions());
  variant->features = ReadFlags(def->features());

  filament::MaterialInstance* default_instance =
      variant->fmaterial->getDefaultInstance();

  if (def->properties()) {
    variant->params.reserve(def->properties()->size());
    for (const ShaderPropertyAssetDef* property : *def->properties()) {
      CHECK(property->name() && property->name()->name());
      ParameterInfo param;
      param.name = property->name()->name()->str();
      param.key = HashValue(property->name()->hash());
      param.type = property->type();
      if (property->texture_usage()) {
        CHECK(param.type == MaterialPropertyType::Sampler2D ||
              param.type == MaterialPropertyType::SamplerCubeMap);
        param.texture_usage = TextureUsage(*property->texture_usage());
        param.key = param.texture_usage.Hash();
      }
      variant->params.emplace_back(param);

      if (property->default_floats()) {
        absl::Span<const std::byte> data = AsBytes(*property->default_floats());
        SetParameter(default_instance, param.name.c_str(), param.type, data);
      } else if (property->default_ints()) {
        absl::Span<const std::byte> data = AsBytes(*property->default_ints());
        SetParameter(default_instance, param.name.c_str(), param.type, data);
      }
    }
  }

  variants_.emplace_back(std::move(variant));
}

FilamentShader::VariantId FilamentShader::DetermineVariantId(
    const FlagSet& conditions, const FlagSet& features) const {
  for (int i = 0; i < variants_.size(); ++i) {
    const auto& variant = variants_[i];
    if (absl::c_includes(conditions, variant->conditions) &&
        absl::c_includes(variant->features, features)) {
      return i;
    }
  }
  LOG(ERROR) << "Unable to find matching shader variant, falling back to "
                "simplest variant.";
  return static_cast<VariantId>(variants_.size()) - 1;
}

const filament::Material* FilamentShader::GetFilamentMaterial(
    VariantId id) const {
  CHECK(id >= 0 && id < variants_.size());
  auto& variant = variants_[id];
  return variant->fmaterial.get();
}

void FilamentShader::ForEachParameter(VariantId id, const ForParameterFn& fn) {
  CHECK(id >= 0 && id < variants_.size());
  auto& variant = variants_[id];
  for (const ParameterInfo& param : variant->params) {
    fn(param);
  }
}

void FilamentShader::SetParameter(filament::MaterialInstance* instance,
                                  const char* name, MaterialPropertyType type,
                                  absl::Span<const std::byte> data) {
  using filament::math::float2;
  using filament::math::float3;
  using filament::math::float4;
  switch (type) {
    case MaterialPropertyType::Float1: {
      CHECK_EQ(data.size(), sizeof(float));
      const float* value = reinterpret_cast<const float*>(data.data());
      instance->setParameter(name, *value);
      return;
    }
    case MaterialPropertyType::Float2: {
      CHECK_EQ(data.size(), sizeof(float2));
      const float2* value = reinterpret_cast<const float2*>(data.data());
      instance->setParameter(name, *value);
      return;
    }
    case MaterialPropertyType::Float3: {
      CHECK_EQ(data.size(), sizeof(float3));
      const float3* value = reinterpret_cast<const float3*>(data.data());
      instance->setParameter(name, *value);
      return;
    }
    case MaterialPropertyType::Float4: {
      CHECK_EQ(data.size(), sizeof(float4));
      const float4* value = reinterpret_cast<const float4*>(data.data());
      instance->setParameter(name, *value);
      return;
    }
    default:
      LOG(FATAL) << "Unsupported material type: " << ToString(type);
  }
}

void FilamentShader::SetParameter(filament::MaterialInstance* instance,
                                  const char* name, MaterialPropertyType type,
                                  const TexturePtr& texture) {
  FilamentTexture* impl = static_cast<FilamentTexture*>(texture.get());
  if (impl->IsReady()) {
    instance->setParameter(name, impl->GetFilamentTexture(),
                           impl->GetFilamentSampler());
  }
}

}  // namespace redux
