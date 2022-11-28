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

#include "redux/tools/shader_pipeline/shader_pipeline.h"

#include <memory>

#include "filamat/MaterialBuilder.h"
#include "filamat/Package.h"
#include "utils/JobSystem.h"
#include "redux/data/asset_defs/shader_asset_def_generated.h"
#include "redux/modules/base/data_builder.h"
#include "redux/modules/flatbuffers/common.h"
#include "redux/modules/graphics/enums.h"
#include "redux/modules/graphics/texture_usage.h"
#include "redux/tools/common/file_utils.h"
#include "redux/tools/common/flatbuffer_utils.h"

namespace redux::tool {

static filament::Shading ToFilament(ShaderAsset::Shading value) {
  switch (value) {
    case ShaderAsset::Unlit:
      return filament::Shading::UNLIT;
    case ShaderAsset::Lit:
      return filament::Shading::LIT;
    case ShaderAsset::Cloth:
      return filament::Shading::CLOTH;
    case ShaderAsset::Subsurface:
      return filament::Shading::SUBSURFACE;
    case ShaderAsset::SpecularGlossiness:
      return filament::Shading::SPECULAR_GLOSSINESS;
  }
}

static filament::BlendingMode ToFilament(ShaderAsset::BlendMode value) {
  switch (value) {
    case ShaderAsset::Opaque:
      return filament::BlendingMode::OPAQUE;
    case ShaderAsset::Transparent:
      return filament::BlendingMode::TRANSPARENT;
    case ShaderAsset::Fade:
      return filament::BlendingMode::FADE;
    case ShaderAsset::Add:
      return filament::BlendingMode::ADD;
    case ShaderAsset::Masked:
      return filament::BlendingMode::MASKED;
    case ShaderAsset::Multiply:
      return filament::BlendingMode::MULTIPLY;
    case ShaderAsset::Screen:
      return filament::BlendingMode::SCREEN;
  }
}

static filament::TransparencyMode ToFilament(
    ShaderAsset::TransparencyMode value) {
  switch (value) {
    case ShaderAsset::Default:
      return filament::TransparencyMode::DEFAULT;
    case ShaderAsset::TwoPassesOneSide:
      return filament::TransparencyMode::TWO_PASSES_ONE_SIDE;
    case ShaderAsset::TwoPassesTwoSides:
      return filament::TransparencyMode::TWO_PASSES_TWO_SIDES;
  }
}

static filamat::MaterialBuilder::CullingMode ToFilament(
    ShaderAsset::CullingMode value) {
  switch (value) {
    case ShaderAsset::None:
      return filamat::MaterialBuilder::CullingMode::NONE;
    case ShaderAsset::FrontFace:
      return filamat::MaterialBuilder::CullingMode::FRONT;
    case ShaderAsset::BackFace:
      return filamat::MaterialBuilder::CullingMode::BACK;
    case ShaderAsset::FrontAndBack:
      return filamat::MaterialBuilder::CullingMode::FRONT_AND_BACK;
  }
}

static filamat::MaterialBuilder::UniformType ToFilament(
    MaterialPropertyType value) {
  switch (value) {
    case MaterialPropertyType::Float1:
      return filamat::MaterialBuilder::UniformType::FLOAT;
    case MaterialPropertyType::Float2:
      return filamat::MaterialBuilder::UniformType::FLOAT2;
    case MaterialPropertyType::Float3:
      return filamat::MaterialBuilder::UniformType::FLOAT3;
    case MaterialPropertyType::Float4:
      return filamat::MaterialBuilder::UniformType::FLOAT4;
    default:
      LOG(FATAL) << "Unsupported property type: " << ToString(value);
  }
}

static void SetupShader(const ShaderAsset& asset,
                        filamat::MaterialBuilder* builder) {
  builder->name(asset.name.c_str());
  builder->platform(filamat::MaterialBuilder::Platform::ALL);
  builder->targetApi(filamat::MaterialBuilder::TargetApi::ALL);
  builder->colorWrite(asset.color_write);
  builder->depthWrite(asset.depth_write);
  builder->depthCulling(asset.depth_cull);
  builder->doubleSided(asset.double_sided);
  builder->shading(ToFilament(asset.shading));
  builder->transparencyMode(ToFilament(asset.transparency));
  builder->culling(ToFilament(asset.culling));
  builder->blending(ToFilament(asset.blending));
  builder->postLightingBlending(ToFilament(asset.post_lighting_blending));
  builder->flipUV(false);
  builder->optimization(filamat::MaterialBuilder::Optimization::NONE);
}

// Assigns MaterialBuilder vertex attributes using data from the ShaderAsset.
static void SetupAttributes(const ShaderAsset& asset,
                            filamat::MaterialBuilder* builder) {
  for (VertexUsage usage : asset.vertex_attributes) {
    switch (usage) {
      case VertexUsage::Position:
        builder->require(filament::VertexAttribute::POSITION);
        break;
      case VertexUsage::Orientation:
        builder->require(filament::VertexAttribute::TANGENTS);
        break;
      case VertexUsage::Color0:
        builder->require(filament::VertexAttribute::COLOR);
        break;
      case VertexUsage::TexCoord0:
        builder->require(filament::VertexAttribute::UV0);
        break;
      case VertexUsage::TexCoord1:
        builder->require(filament::VertexAttribute::UV1);
        break;
      case VertexUsage::BoneWeights:
        builder->require(filament::VertexAttribute::BONE_WEIGHTS);
        break;
      case VertexUsage::BoneIndices:
        builder->require(filament::VertexAttribute::BONE_INDICES);
        break;
      default:
        LOG(FATAL) << "Unsupported vertex usage: " << ToString(usage);
    }
  }
}

// Assigns MaterialBuilder parameters using data from the ShaderAsset.
static void SetupParameters(const ShaderAsset& asset,
                            filamat::MaterialBuilder* builder) {
  for (const ShaderAssetParameter& parameter : asset.parameters) {
    const int count = parameter.array_size;
    const char* name = parameter.name.c_str();
    if (parameter.type == MaterialPropertyType::Sampler2D) {
      builder->parameter(name,
                         filamat::MaterialBuilder::SamplerType::SAMPLER_2D);
    } else if (count > 0) {
      builder->parameter(name, count, ToFilament(parameter.type));
    } else {
      builder->parameter(name, ToFilament(parameter.type));
    }
  }
}

// Extracts a list of (hashed) features from the ShaderAsset.
static std::vector<uint32_t> GatherFeatures(const ShaderAsset& asset) {
  std::vector<uint32_t> features;
  for (const auto& f : asset.features) {
    features.emplace_back(Hash(f).get());
  }
  return features;
}

// Extracts a list of (hashed) conditions from the ShaderAsset.
static std::vector<uint32_t> GatherConditions(const ShaderAsset& asset) {
  std::vector<uint32_t> conditions;
  for (const auto& a : asset.vertex_attributes) {
    conditions.emplace_back(Hash(a).get());
  }
  for (const auto& p : asset.parameters) {
    if (!p.texture_usage.empty()) {
      TextureUsage usage(p.texture_usage);
      conditions.emplace_back(usage.Hash().get());
    }
  }
  return conditions;
}

static std::string BuildGlslHeader(const ShaderAsset& asset) {
  std::stringstream ss;
  for (const std::string& define : asset.defines) {
    ss << "#define " << define << std::endl;
  }
  ss << std::endl;
  return ss.str();
}

std::unique_ptr<ShaderVariantAssetDefT> BuildVariant(const ShaderAsset& asset) {
  filamat::MaterialBuilder builder;

  auto includer = [](const utils::CString& includedBy,
                     filamat::IncludeResult& result) {
    const std::string contents = LoadFileAsString(result.includeName.c_str());
    result.name = utils::CString(result.includeName.c_str());
    result.text = utils::CString(contents.c_str());
    return !contents.empty();
  };
  builder.includeCallback(includer);

  const std::string preamble = BuildGlslHeader(asset);

  if (!asset.vertex_shader.empty()) {
    const std::string vertex = LoadFileAsString(asset.vertex_shader.c_str());
    builder.materialVertex((preamble + vertex).c_str());
  }

  const std::string fragment = LoadFileAsString(asset.fragment_shader.c_str());
  builder.material((preamble + fragment).c_str());

  SetupShader(asset, &builder);
  SetupAttributes(asset, &builder);
  SetupParameters(asset, &builder);

  // Build the filament material package.
  utils::JobSystem js;
  js.adopt();
  filamat::Package package = builder.build(js);
  CHECK(package.isValid());
  js.emancipate();

  auto variant = std::make_unique<ShaderVariantAssetDefT>();
  variant->name = asset.name;
  variant->filament_material.assign(package.getData(), package.getEnd());
  variant->conditions = GatherConditions(asset);
  variant->features = GatherFeatures(asset);

  variant->properties.reserve(asset.parameters.size());
  for (const ShaderAssetParameter& param : asset.parameters) {
    auto property = std::make_unique<ShaderPropertyAssetDefT>();
    property->name =
        std::make_unique<fbs::HashStringT>(CreateHashStringT(param.name));
    property->type = param.type;
    property->texture_usage = param.texture_usage;
    property->default_ints = param.default_ints;
    property->default_floats = param.default_floats;
    variant->properties.emplace_back(std::move(property));
  }

  return variant;
}

DataContainer BuildShader(std::string_view name,
                          absl::Span<const ShaderAsset> assets) {
  ShaderAssetDefT shader_def;
  shader_def.shading_model = std::string(name);

  filamat::MaterialBuilder::init();
  for (const ShaderAsset& asset : assets) {
    shader_def.variants.emplace_back(BuildVariant(asset));
  }
  filamat::MaterialBuilder::shutdown();
  return BuildFlatbuffer(shader_def);
}

}  // namespace redux::tool
