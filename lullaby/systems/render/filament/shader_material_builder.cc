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

#include "lullaby/systems/render/filament/shader_material_builder.h"
#include "lullaby/generated/flatbuffers/vertex_attribute_def_generated.h"
#include "lullaby/util/registry.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#include "filamat/MaterialBuilder.h"
#include "lullaby/generated/flatbuffers/material_def_generated.h"
#include "lullaby/generated/flatbuffers/shader_def_generated.h"
#include "lullaby/generated/flatbuffers/texture_def_generated.h"
#include "lullaby/modules/render/material_info.h"
#include "lullaby/modules/render/shader_description.h"
#include "lullaby/modules/render/shader_snippets_selector.h"
#include "lullaby/util/filename.h"
#include "lullaby/util/logging.h"

namespace lull {

static constexpr filamat::MaterialBuilderBase::TargetApi g_filamat_api =
    filamat::MaterialBuilderBase::TargetApi::OPENGL;

static constexpr filamat::MaterialBuilderBase::Platform g_filamat_platform =
#if defined(__ANDROID__) || TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR || \
    defined(__EMSCRIPTEN__)
    filamat::MaterialBuilderBase::Platform::MOBILE;
#else
    filamat::MaterialBuilderBase::Platform::DESKTOP;
#endif

static const char* ToFilamentCodeString(VertexAttributeType type) {
  switch (type) {
    case VertexAttributeType_Empty:
      LOG(DFATAL) << "Empty vertex attribute.";
      return "empty";
    case VertexAttributeType_Scalar1f:
      return "float";
    case VertexAttributeType_Vec2f:
      return "vec2";
    case VertexAttributeType_Vec3f:
      return "vec3";
    case VertexAttributeType_Vec4f:
      return "vec4";
    case VertexAttributeType_Vec2us:
      return "uvec2";
    case VertexAttributeType_Vec4us:
      return "uvec4";
    case VertexAttributeType_Vec4ub:
      return "bvec4";
  }
}

static const char* ToFilamentCodeString(VertexAttributeUsage usage, int index) {
  switch (usage) {
    case VertexAttributeUsage_Invalid:
      LOG(ERROR) << "Invalid vertex attribute usage.";
      return "_Invalid";
    case VertexAttributeUsage_Position:
      return "getSkinnedPosition()";
    case VertexAttributeUsage_Color:
      return "getColor()";
    case VertexAttributeUsage_TexCoord:
      if (index == 0) {
        return "vec2(getUV0().x, 1. - getUV0().y)";
      } else {
        return "vec2(getUV1().x, 1. - getUV1().y)";
      }
    case VertexAttributeUsage_Normal:
      return "getWorldNormalVector()";
    case VertexAttributeUsage_Tangent:
      return "getWorldTangentFrame()";
    case VertexAttributeUsage_Orientation:
      LOG(DFATAL) << "Not implemented.";
      return "_Orientation";
    case VertexAttributeUsage_BoneIndices:
      LOG(DFATAL) << "Not implemented.";
      return "_BoneIndices";
    case VertexAttributeUsage_BoneWeights:
      LOG(DFATAL) << "Not implemented.";
      return "_BoneWeights";
  }
}

static ShaderDataType ToShaderDataType(filament::Material::ParameterType type) {
  switch (type) {
    case filament::Material::ParameterType::FLOAT:
      return ShaderDataType_Float1;
    case filament::Material::ParameterType::FLOAT2:
      return ShaderDataType_Float2;
    case filament::Material::ParameterType::FLOAT3:
      return ShaderDataType_Float3;
    case filament::Material::ParameterType::FLOAT4:
      return ShaderDataType_Float4;
    case filament::Material::ParameterType::INT:
      return ShaderDataType_Int1;
    case filament::Material::ParameterType::INT2:
      return ShaderDataType_Int2;
    case filament::Material::ParameterType::INT3:
      return ShaderDataType_Int3;
    case filament::Material::ParameterType::INT4:
      return ShaderDataType_Int4;
    case filament::Material::ParameterType::MAT3:
      return ShaderDataType_Float3x3;
    case filament::Material::ParameterType::MAT4:
      return ShaderDataType_Float4x4;
    default:
      LOG(ERROR) << "Unsupported property type: " << static_cast<int>(type);
      return ShaderDataType_MAX;
  }
}

static TextureUsageInfo ToTextureUsageInfo(const std::string& name) {
  const auto& values = EnumValuesMaterialTextureUsage();
  const size_t num_values = sizeof(values) / sizeof(values[0]);
  for (size_t i = 0; i < num_values; ++i) {
    const MaterialTextureUsage usage = values[i];
    if (name == std::string(EnumNameMaterialTextureUsage(usage))) {
      return TextureUsageInfo(usage);
    }
  }
  return TextureUsageInfo(MaterialTextureUsage_Unused);
}

static void AddUniform(ShaderDescription* desc, std::string name,
                       ShaderDataType type) {
  ShaderUniformDefT uniform;
  uniform.name = std::move(name);
  uniform.type = type;
  desc->uniforms.push_back(std::move(uniform));
}

static void AddSampler(ShaderDescription* desc, std::string name,
                       TextureUsageInfo usage) {
  ShaderSamplerDefT sampler;
  sampler.name = std::move(name);
  sampler.type = TextureTargetType_Standard2d;

  if (usage.GetChannelUsage(1) == MaterialTextureUsage_Unused &&
      usage.GetChannelUsage(2) == MaterialTextureUsage_Unused &&
      usage.GetChannelUsage(2) == MaterialTextureUsage_Unused) {
    sampler.usage = usage.GetChannelUsage(0);
  } else {
    sampler.usage_per_channel.push_back(usage.GetChannelUsage(0));
    sampler.usage_per_channel.push_back(usage.GetChannelUsage(1));
    sampler.usage_per_channel.push_back(usage.GetChannelUsage(2));
    sampler.usage_per_channel.push_back(usage.GetChannelUsage(3));
  }
  desc->samplers.push_back(std::move(sampler));
}

static void AddAttribute(ShaderDescription* desc, std::string name,
                         VertexAttributeType type, VertexAttributeUsage usage) {
  ShaderAttributeDefT attrib;
  attrib.name = std::move(name);
  attrib.type = type;
  attrib.usage = usage;
  desc->attributes.push_back(std::move(attrib));
}

static bool HasFeature(const ShaderSelectionParams& params,
                       TextureUsageInfo usage) {
  return params.features.count(usage.GetHash()) > 0;
}

static bool HasFeature(const ShaderSelectionParams& params, HashValue feature) {
  return params.features.count(feature) > 0 ||
         params.environment.count(feature) > 0;
}

bool HasUniform(const ShaderDescription& desc, HashValue name) {
  for (auto& uniform : desc.uniforms) {
    if (Hash(uniform.name) == name) {
      return true;
    }
  }
  return false;
}

bool HasSampler(const ShaderDescription& desc, HashValue name) {
  for (auto& sampler : desc.samplers) {
    if (Hash(sampler.name) == name) {
      return true;
    }
  }
  return false;
}

bool HasAttribute(const ShaderDescription& desc, HashValue name) {
  for (auto& attribute : desc.attributes) {
    if (Hash(attribute.name) == name) {
      return true;
    }
  }
  return false;
}

static void InitDescriptionFromParams(ShaderDescription* desc,
                                      const std::string& shading_model,
                                      const ShaderSelectionParams& params) {
  static const TextureUsageInfo kBaseColorUsage(MaterialTextureUsage_BaseColor);
  static const TextureUsageInfo kNormalUsage(MaterialTextureUsage_Normal);
  static const TextureUsageInfo kEmissiveUsage(MaterialTextureUsage_Emissive);
  static const TextureUsageInfo kOcclusionUsage(MaterialTextureUsage_Occlusion);
  static const TextureUsageInfo kRoughnessMetallic(
      {MaterialTextureUsage_Unused, MaterialTextureUsage_Roughness,
       MaterialTextureUsage_Metallic});
  static const TextureUsageInfo kORMUsage({MaterialTextureUsage_Occlusion,
                                           MaterialTextureUsage_Roughness,
                                           MaterialTextureUsage_Metallic});

  desc->shading_model = shading_model;
  AddUniform(desc, "color", ShaderDataType_Float4);

  if (HasFeature(params, ConstHash("BaseColor"))) {
    AddUniform(desc, "BaseColor", ShaderDataType_Float4);
  }
  if (HasFeature(params, ConstHash("Emissive"))) {
    AddUniform(desc, "Emissive", ShaderDataType_Float4);
  }
  if (HasFeature(params, ConstHash("Metallic"))) {
    AddUniform(desc, "Metallic", ShaderDataType_Float1);
  }
  if (HasFeature(params, ConstHash("Roughness"))) {
    AddUniform(desc, "Roughness", ShaderDataType_Float1);
  }
  if (HasFeature(params, ConstHash("Smoothness"))) {
    AddUniform(desc, "Smoothness", ShaderDataType_Float1);
  }
  if (HasFeature(params, ConstHash("Occlusion"))) {
    AddUniform(desc, "Occlusion", ShaderDataType_Float1);
  }
  if (HasFeature(params, kBaseColorUsage)) {
    AddSampler(desc, "BaseColorMap", kBaseColorUsage);
  }
  if (HasFeature(params, kNormalUsage)) {
    AddSampler(desc, "NormalMap", kNormalUsage);
  }
  if (HasFeature(params, kEmissiveUsage)) {
    AddSampler(desc, "EmissiveMap", kEmissiveUsage);
  }
  if (HasFeature(params, kOcclusionUsage)) {
    AddSampler(desc, "OcclusionMap", kOcclusionUsage);
  }
  if (HasFeature(params, kRoughnessMetallic)) {
    AddSampler(desc, "RoughnessMetallicMap", kRoughnessMetallic);
  }
  if (HasFeature(params, kORMUsage)) {
    AddSampler(desc, "OrmMap", kORMUsage);
  }
  if (HasFeature(params, ConstHash("Transform"))) {
    AddAttribute(desc, "Transform", VertexAttributeType_Vec3f,
                 VertexAttributeUsage_Position);
  }
  if (HasFeature(params, ConstHash("VertexColor"))) {
    AddAttribute(desc, "VertexColor", VertexAttributeType_Vec4ub,
                 VertexAttributeUsage_Color);
  }
  if (HasFeature(params, ConstHash("Texture"))) {
    AddAttribute(desc, "Texture", VertexAttributeType_Vec2f,
                 VertexAttributeUsage_TexCoord);
  }
  if (HasFeature(params, ConstHash("Texture1"))) {
    AddAttribute(desc, "Texture1", VertexAttributeType_Vec2f,
                 VertexAttributeUsage_TexCoord);
  }
  if (HasFeature(params, ConstHash("Skin"))) {
    AddAttribute(desc, "BoneIndices", VertexAttributeType_Vec4ub,
                 VertexAttributeUsage_BoneIndices);
  }
  if (HasFeature(params, ConstHash("Skin"))) {
    AddAttribute(desc, "BoneWeights", VertexAttributeType_Vec4ub,
                 VertexAttributeUsage_BoneWeights);
  }
}

void InitDescriptionFromMaterial(ShaderDescription* desc,
                                 const filament::Material* material) {
  std::vector<filament::Material::ParameterInfo> params;
  params.resize(material->getParameterCount());
  material->getParameters(params.data(), params.size());

  for (const filament::Material::ParameterInfo& info : params) {
    if (info.isSampler) {
      AddSampler(desc, info.name, ToTextureUsageInfo(info.name));
    } else {
      AddUniform(desc, info.name, ToShaderDataType(info.type));
    }
  }

  const auto attribs = material->getRequiredAttributes();
  if (attribs.test(filament::VertexAttribute::POSITION)) {
    AddAttribute(desc, "Transform", VertexAttributeType_Vec3f,
                 VertexAttributeUsage_Position);
  }
  if (attribs.test(filament::VertexAttribute::TANGENTS)) {
    AddAttribute(desc, "Tangents", VertexAttributeType_Vec4f,
                 VertexAttributeUsage_Orientation);
  }
  if (attribs.test(filament::VertexAttribute::COLOR)) {
    AddAttribute(desc, "VertexColor", VertexAttributeType_Vec4ub,
                 VertexAttributeUsage_Color);
  }
  if (attribs.test(filament::VertexAttribute::UV0)) {
    AddAttribute(desc, "Texture", VertexAttributeType_Vec2f,
                 VertexAttributeUsage_TexCoord);
  }
  if (attribs.test(filament::VertexAttribute::UV1)) {
    AddAttribute(desc, "Texture1", VertexAttributeType_Vec2f,
                 VertexAttributeUsage_TexCoord);
  }
  if (attribs.test(filament::VertexAttribute::BONE_INDICES)) {
    AddAttribute(desc, "BoneIndices", VertexAttributeType_Vec4ub,
                 VertexAttributeUsage_BoneIndices);
  }
  if (attribs.test(filament::VertexAttribute::BONE_WEIGHTS)) {
    AddAttribute(desc, "BoneWeights", VertexAttributeType_Vec4ub,
                 VertexAttributeUsage_BoneWeights);
  }
}

static void SetupMaterialSamplers(filamat::MaterialBuilder* builder,
                                  const ShaderDescription& desc) {
  // Create filament sampler properties for all the samplers in the shader
  // description.
  using SamplerType = filamat::MaterialBuilder::SamplerType;
  for (auto& sampler : desc.samplers) {
    builder->parameter(SamplerType::SAMPLER_2D, sampler.name.c_str());
  }
}

static void SetupMaterialUniforms(filamat::MaterialBuilder* builder,
                                  const ShaderDescription& desc) {
  // Create filament material properties for all the uniforms in the
  // shader description.
  using UniformType = filamat::MaterialBuilder::UniformType;
  for (auto& uniform : desc.uniforms) {
    const int count = uniform.array_size;
    if (count > 0) {
      switch (uniform.type) {
        case ShaderDataType_Float1:
          builder->parameter(UniformType::FLOAT, count, uniform.name.c_str());
          break;
        case ShaderDataType_Float2:
          builder->parameter(UniformType::FLOAT2, count, uniform.name.c_str());
          break;
        case ShaderDataType_Float3:
          builder->parameter(UniformType::FLOAT3, count, uniform.name.c_str());
          break;
        case ShaderDataType_Float4:
          builder->parameter(UniformType::FLOAT4, count, uniform.name.c_str());
          break;
        default:
          break;
      }
    } else {
      switch (uniform.type) {
        case ShaderDataType_Float1:
          builder->parameter(UniformType::FLOAT, uniform.name.c_str());
          break;
        case ShaderDataType_Float2:
          builder->parameter(UniformType::FLOAT2, uniform.name.c_str());
          break;
        case ShaderDataType_Float3:
          builder->parameter(UniformType::FLOAT3, uniform.name.c_str());
          break;
        case ShaderDataType_Float4:
          builder->parameter(UniformType::FLOAT4, uniform.name.c_str());
          break;
        default:
          break;
      }
    }
  }
}

static void SetupMaterial(filamat::MaterialBuilder* builder,
                          const ShaderDescription& desc) {
  builder->name(desc.shading_model.c_str());
  builder->platform(g_filamat_platform);
  builder->targetApi(g_filamat_api);
  builder->blending(filament::BlendingMode::OPAQUE);
  builder->optimization(filamat::MaterialBuilder::Optimization::NONE);
  if (HasUniform(desc, ConstHash("sdf_params"))) {
    builder->shading(filament::Shading::LIT);
    builder->blending(filament::BlendingMode::TRANSPARENT);
  } else {
    builder->shading(filament::Shading::LIT);
  }

  if (HasAttribute(desc, ConstHash("VertexColor"))) {
    builder->require(filament::VertexAttribute::COLOR);
  }
  if (HasAttribute(desc, ConstHash("Tangents"))) {
    builder->require(filament::VertexAttribute::TANGENTS);
  }
  if (HasAttribute(desc, ConstHash("Texture"))) {
    builder->require(filament::VertexAttribute::UV0);
  }
  if (HasAttribute(desc, ConstHash("Texture1"))) {
    builder->require(filament::VertexAttribute::UV1);
  }
  if (HasAttribute(desc, ConstHash("BoneIndices"))) {
    builder->require(filament::VertexAttribute::BONE_INDICES);
  }
  if (HasAttribute(desc, ConstHash("BoneWeights"))) {
    builder->require(filament::VertexAttribute::BONE_WEIGHTS);
  }

  SetupMaterialUniforms(builder, desc);
  SetupMaterialSamplers(builder, desc);
}

static void LogGeneratedCode(std::string source) {
  int linenum = 0;
  std::stringstream tmp;
  while (true) {
    auto pos = source.find("\n");
    if (pos == std::string::npos) {
      break;
    }
    tmp << linenum << " :  " << source.substr(0, pos) << std::endl;
    source = source.substr(pos + 1);
    ++linenum;
  }
  LOG(INFO) << "\n" << tmp.str();
}

std::string ShaderMaterialBuilder::BuildFragmentCode(const ShaderStage& stage) {
  if (stage.outputs.size() > 1) {
    LOG(FATAL) << "Can only output a single variable!";
    return "";
  } else if (stage.outputs.size() == 1) {
    if (stage.outputs[0].type != VertexAttributeType_Vec4f) {
      LOG(FATAL) << "Output must be type vec4!";
      return "";
    }
  }

  std::stringstream ss;
  ss << "#define UNIFORM(name) materialParams.name\n";
  ss << "#define SAMPLER(name) materialParams_##name\n";
  ss << "\n";

  // Create global variables for inputs and outputs.
  ss << "// Stage inputs.\n";
  for (const auto& input : stage.inputs) {
    ss << ToFilamentCodeString(input.type) << " " << input.name << ";\n";
  }
  ss << "\n";
  ss << "// Stage outputs.\n";
  for (const auto& output : stage.outputs) {
    ss << ToFilamentCodeString(output.type) << " " << output.name << ";\n";
  }
  ss << "\n";

  for (const auto& code : stage.code) {
    ss << code << "\n";
  }

  if (!stage.main.empty()) {
    int function_count = 0;
    for (const auto& main : stage.main) {
      ss << "// " << stage.snippet_names[function_count] << "\n";
      ss << "void GeneratedFunction" << function_count << "() {\n";

      size_t prev = 0;
      for (size_t i = 0; i < main.length(); ++i) {
        if (main[i] == '\n' || main[i] == '\r') {
          ss << "  " << std::string(main.substr(prev, i - prev)) << "\n";
          prev = i + 1;
        }
      }
      // ss << "  " << std::string(main.substr(prev)) << "\n";
      ss << "}\n\n";

      ++function_count;
    }

    ss << "void material(inout MaterialInputs material) {\n";

    // Copy filament values into global variables, eg: color = getColor();
    int texture_count = 0;
    for (const auto& input : stage.inputs) {
      if (input.usage != VertexAttributeUsage_Invalid) {
        int index = 0;
        if (input.usage == VertexAttributeUsage_TexCoord) {
          index = texture_count;
          ++texture_count;
        }
        const char* usage = ToFilamentCodeString(input.usage, index);
        ss << "  " << input.name << " = " << usage << ";\n";
      }
    }

    ss << "  prepareMaterial(material);\n\n";

    for (int i = 0; i < function_count; ++i) {
      ss << "  // " << stage.snippet_names[i] << "\n";
      ss << "  GeneratedFunction" << i << "();\n\n";
    }

    if (stage.outputs.size() == 1) {
      ss << "  material.baseColor = " << stage.outputs[0].name << ";\n";
    }
    ss << "}\n";
  }

  return ss.str();
}

std::string ShaderMaterialBuilder::BuildFragmentCode(
    const ShaderDescription& desc) {
  std::stringstream ss;
  ss << "void material(inout MaterialInputs material) {\n";

  if (HasAttribute(desc, ConstHash("Texture"))) {
    ss << "  vec2 uv0 = getUV0();\n";
    ss << "  uv0.y = 1. - uv0.y;\n";
  }
  if (HasAttribute(desc, ConstHash("Texture1"))) {
    ss << "  vec2 uv1 = getUV1();\n";
    ss << "  uv1.y = 1. - uv1.y;\n";
  }

  // Normals.
  if (HasSampler(desc, ConstHash("NormalMap")) &&
      HasAttribute(desc, ConstHash("Texture"))) {
    ss << "  material.normal = texture(materialParams_NormalMap, uv0).xyz;\n";
    ss << "  material.normal *= 2.0;\n";
    ss << "  material.normal -= 1.0;\n";
  }

  ss << "  prepareMaterial(material);\n";

  // BaseColor.
  ss << "  material.baseColor = vec4(1);\n";
  if (HasUniform(desc, ConstHash("color"))) {
    ss << "  material.baseColor *= materialParams.color;\n";
  }
  if (HasUniform(desc, ConstHash("BaseColor"))) {
    ss << "  material.baseColor *= materialParams.BaseColor;\n";
  }
  if (HasSampler(desc, ConstHash("BaseColorMap")) &&
      HasAttribute(desc, ConstHash("Texture"))) {
    ss << "  material.baseColor *= texture(materialParams_BaseColorMap,uv0);\n";
  }

  // Emissive.
  if (HasUniform(desc, ConstHash("Emissive"))) {
    ss << "  material.emissive = materialParams.Emissive;\n";
  }
  if (HasSampler(desc, ConstHash("EmissiveMap")) &&
      HasAttribute(desc, ConstHash("Texture"))) {
    ss << "  material.emissive *= texture(materialParams_EmissiveMap, uv0);\n";
  }

  // Metallic-Roughness-Occlusion.
  if (HasUniform(desc, ConstHash("Roughness"))) {
    ss << "  material.roughness = materialParams.Roughness;\n";
  } else if (HasUniform(desc, ConstHash("Smoothness"))) {
    ss << "  material.roughness = 1.0 - materialParams.Smoothness;\n";
  }
  if (HasUniform(desc, ConstHash("Metallic"))) {
    ss << "  material.metallic = materialParams.Metallic;\n";
  }
  if (HasSampler(desc, ConstHash("OcclusionMap")) &&
      HasAttribute(desc, ConstHash("Texture"))) {
    ss << "  float occlusion = texture(materialParams_OcclusionMap, uv0).r;\n";
    ss << "  material.ambientOcclusion *= occlusion;\n";
  }
  if (HasSampler(desc, ConstHash("RoughnessMetallicMap")) &&
      HasAttribute(desc, ConstHash("Texture"))) {
    ss << "  vec2 rm = texture(materialParams_RoughnessMetallicMap, uv0).gb;\n";
    ss << "  material.roughness *= rm.r;\n";
    ss << "  material.metallic *= rm.g;\n";
  }
  if (HasSampler(desc, ConstHash("OrmMap")) &&
      HasAttribute(desc, ConstHash("Texture"))) {
    ss << "  vec3 orm = texture(materialParams_OrmMap, uv0).rgb;\n";
    ss << "  material.ambientOcclusion *= orm.r;\n";
    ss << "  material.roughness *= orm.g;\n";
    ss << "  material.metallic *= orm.b;\n";
  }

  // Text.
  if (HasUniform(desc, ConstHash("sdf_params"))) {
    // clang-format off
    // Autoformatting strings doesn't really help with formatting the code
    // contained in the strings so disable the formatter.
    ss << "  #define kSDFTextureUnitDistancePerTexel (1. / 8.)\n";
    ss << "  #define kSDFTransitionValue .5\n";

    ss << "  vec2 uv_texel = uv0 * vec2(textureSize(materialParams_BaseColor, 0));\n";
    ss << "  vec2 width = fwidth(uv_texel);\n";
    ss << "  float uv_rate = max(width.x, width.y);\n";

    ss << "  float sdf_dist = (1. - texture(materialParams_BaseColor, uv0).r) - kSDFTransitionValue;\n";
    ss << "  float sdf_dist_rate = uv_rate * kSDFTextureUnitDistancePerTexel;\n";
    ss << "  float alpha = sdf_dist / sdf_dist_rate + .5;\n";
    ss << "  alpha = clamp(alpha, 0., 1.);\n";
    ss << "  material.baseColor = materialParams.color * alpha;\n";
    // clang-format on
  }

  ss << "}\n";
  return ss.str();
}

ShaderMaterialBuilder::ShaderMaterialBuilder(
    filament::Engine* engine, const std::string& shading_model,
    const ShaderDefT* shader_def, const ShaderSelectionParams& params)
    : engine_(engine), selection_params_(&params) {
  filamat::MaterialBuilder builder;

  std::string code;
  if (shader_def) {
    // Run the snippet selection logic
    const SnippetSelectionResult snippets =
        SelectShaderSnippets(*shader_def, params);

    // Generate the shader description using the selected snippets.
    description_ = CreateShaderDescription(shading_model, snippets.stages);

    // Build the material data package.
    SetupMaterial(&builder, description_);
    code = BuildFragmentCode(snippets.stages[ShaderStageType_Fragment]);
  } else {
    // Generate the shader description directly from the shader parameters.
    InitDescriptionFromParams(&description_, shading_model, params);

    // Build the material data package.
    SetupMaterial(&builder, description_);
    code = BuildFragmentCode(description_);
  }

#if defined(SHADER_DEBUG)
  LogGeneratedCode(code);
#endif

  builder.material(code.c_str());

  // Build the material.
  filamat::Package pkg = builder.build();
  material_ = filament::Material::Builder()
                  .package(pkg.getData(), pkg.getSize())
                  .build(*engine_);

  if (shader_def) {
    for (auto& uniform : description_.uniforms) {
      if (uniform.values.empty()) {
        continue;
      }

      switch (uniform.type) {
        case ShaderDataType_Float1: {
          float value = uniform.values[0];
          material_->setDefaultParameter(uniform.name.c_str(), value);
          break;
        }
        case ShaderDataType_Float2: {
          filament::math::float2 value{uniform.values[0], uniform.values[1]};
          material_->setDefaultParameter(uniform.name.c_str(), value);
          break;
        }
        case ShaderDataType_Float3: {
          filament::math::float3 value{uniform.values[0], uniform.values[1],
                                       uniform.values[2]};
          material_->setDefaultParameter(uniform.name.c_str(), value);
          break;
        }
        case ShaderDataType_Float4: {
          filament::math::float4 value{uniform.values[0], uniform.values[1],
                                       uniform.values[2], uniform.values[3]};
          material_->setDefaultParameter(uniform.name.c_str(), value);
          break;
        }
        default:
          break;
      }
    }
  }
}

ShaderMaterialBuilder::ShaderMaterialBuilder(filament::Engine* engine,
                                             const std::string& shading_model,
                                             const void* raw_matc_data,
                                             size_t num_bytes)
    : engine_(engine) {
  material_ = filament::Material::Builder()
                  .package(raw_matc_data, num_bytes)
                  .build(*engine_);
  description_.shading_model = shading_model;
  InitDescriptionFromMaterial(&description_, material_);
}

bool ShaderMaterialBuilder::IsValid() const { return material_ != nullptr; }

const ShaderDescription& ShaderMaterialBuilder::GetDescription() const {
  return description_;
}

filament::Material* ShaderMaterialBuilder::GetFilamentMaterial() {
  return material_;
}

const ShaderSelectionParams* ShaderMaterialBuilder::GetShaderSelectionParams()
    const {
  return selection_params_;
}

}  // namespace lull
