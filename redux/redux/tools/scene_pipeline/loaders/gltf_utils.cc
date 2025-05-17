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

#include "redux/tools/scene_pipeline/loaders/gltf_utils.h"

#include <cstdint>
#include <string>

#include "redux/tools/scene_pipeline/drawable.h"
#include "redux/tools/scene_pipeline/material.h"
#include "redux/tools/scene_pipeline/sampler.h"
#include "redux/tools/scene_pipeline/type_id.h"
#include "redux/tools/scene_pipeline/types.h"
#include "redux/tools/scene_pipeline/vertex_buffer.h"
#include "cgltf.h"

namespace redux::tool {

// Constants defined by GLTF/OpenGL.
static constexpr int GL_NEAREST = 0x2600;
static constexpr int GL_LINEAR = 0x2601;
static constexpr int GL_NEAREST_MIPMAP_NEAREST = 0x2700;
static constexpr int GL_LINEAR_MIPMAP_NEAREST = 0x2701;
static constexpr int GL_NEAREST_MIPMAP_LINEAR = 0x2702;
static constexpr int GL_LINEAR_MIPMAP_LINEAR = 0x2703;
static constexpr int GL_CLAMP_TO_EDGE = 0x812F;
static constexpr int GL_MIRRORED_REPEAT = 0x8370;
static constexpr int GL_REPEAT = 0x2901;

int TypeSize(TypeId type) {
  if (type == GetTypeId<std::int8_t>()) {
    return sizeof(std::int8_t);
  } else if (type == GetTypeId<std::int8_t>()) {
    return sizeof(std::int8_t);
  } else if (type == GetTypeId<std::uint8_t>()) {
    return sizeof(std::uint8_t);
  } else if (type == GetTypeId<std::int16_t>()) {
    return sizeof(std::int16_t);
  } else if (type == GetTypeId<std::uint16_t>()) {
    return sizeof(std::uint16_t);
  } else if (type == GetTypeId<std::int32_t>()) {
    return sizeof(std::int32_t);
  } else if (type == GetTypeId<std::uint32_t>()) {
    return sizeof(std::uint32_t);
  } else if (type == GetTypeId<float>()) {
    return sizeof(float);
  } else if (type == GetTypeId<float2>()) {
    return sizeof(float2);
  } else if (type == GetTypeId<float3>()) {
    return sizeof(float3);
  } else if (type == GetTypeId<float4>()) {
    return sizeof(float4);
  } else if (type == GetTypeId<float4x4>()) {
    return sizeof(float4x4);
  } else {
    return 0;
  }
}

TypeId AttributeType(cgltf_type type) {
  switch (type) {
    case cgltf_type_scalar:
      return GetTypeId<float>();
    case cgltf_type_vec2:
      return GetTypeId<float2>();
    case cgltf_type_vec3:
      return GetTypeId<float3>();
    case cgltf_type_vec4:
      return GetTypeId<float4>();
    case cgltf_type_mat4:
      return GetTypeId<float4x4>();
    default:
      return kInvalidTypeId;
  }
}

TypeId ComponentType(cgltf_component_type component_type) {
  switch (component_type) {
    case cgltf_component_type_r_8:
      return GetTypeId<std::int8_t>();
    case cgltf_component_type_r_8u:
      return GetTypeId<std::uint8_t>();
    case cgltf_component_type_r_16:
      return GetTypeId<std::int16_t>();
    case cgltf_component_type_r_16u:
      return GetTypeId<std::uint16_t>();
    case cgltf_component_type_r_32u:
      return GetTypeId<std::uint32_t>();
    case cgltf_component_type_r_32f:
      return GetTypeId<float>();
    default:
      return kInvalidTypeId;
  }
}

Drawable::PrimitiveType PrimitiveType(cgltf_primitive_type type) {
  switch (type) {
    case cgltf_primitive_type_points:
      return Drawable::PrimitiveType::kPointList;
    case cgltf_primitive_type_lines:
      return Drawable::PrimitiveType::kLineList;
    case cgltf_primitive_type_line_loop:
      return Drawable::PrimitiveType::kUnspecified;
    case cgltf_primitive_type_line_strip:
      return Drawable::PrimitiveType::kLineStrip;
    case cgltf_primitive_type_triangles:
      return Drawable::PrimitiveType::kTriangleList;
    case cgltf_primitive_type_triangle_strip:
      return Drawable::PrimitiveType::kTriangleStrip;
    case cgltf_primitive_type_triangle_fan:
      return Drawable::PrimitiveType::kTriangleFan;
    default:
      return Drawable::PrimitiveType::kUnspecified;
  }
}

std::string AttributeName(cgltf_attribute_type type) {
  switch (type) {
    case cgltf_attribute_type_position:
      return std::string(VertexBuffer::kPosition);
    case cgltf_attribute_type_normal:
      return std::string(VertexBuffer::kNormal);
    case cgltf_attribute_type_tangent:
      return std::string(VertexBuffer::kTangent);
    case cgltf_attribute_type_texcoord:
      return std::string(VertexBuffer::kTexCoord);
    case cgltf_attribute_type_color:
      return std::string(VertexBuffer::kColor);
    case cgltf_attribute_type_joints:
      return std::string(VertexBuffer::kBoneIndex);
    case cgltf_attribute_type_weights:
      return std::string(VertexBuffer::kBoneWeight);
    case cgltf_attribute_type_custom:
      return "custom";  // non-standard
    default:
      return "";
  }
}

int AlphaMode(cgltf_alpha_mode mode) {
  switch (mode) {
    case cgltf_alpha_mode_opaque:
      return Material::kAlphaModeOpaque;
    case cgltf_alpha_mode_blend:
      return Material::kAlphaModeBlend;
    case cgltf_alpha_mode_mask:
      return Material::kAlphaModeMask;
    default:
      return Material::kAlphaModeOpaque;
  }
}


Sampler::Filter TextureMinFilter(int filter) {
  switch (filter) {
    case 0:
      return Sampler::Filter::kNearest;
    case GL_NEAREST:
      return Sampler::Filter::kNearest;
    case GL_LINEAR:
      return Sampler::Filter::kLinear;
    case GL_NEAREST_MIPMAP_NEAREST:
      return Sampler::Filter::kNearestMipmapNearest;
    case GL_LINEAR_MIPMAP_NEAREST:
      return Sampler::Filter::kLinearMipmapNearest;
    case GL_NEAREST_MIPMAP_LINEAR:
      return Sampler::Filter::kNearestMipmapLinear;
    case GL_LINEAR_MIPMAP_LINEAR:
      return Sampler::Filter::kLinearMipmapLinear;
    default:
      return Sampler::Filter::kUnspecified;
  }
}

Sampler::Filter TextureMagFilter(int filter) {
  switch (filter) {
    case 0:
      return Sampler::Filter::kNearest;
    case GL_NEAREST:
      return Sampler::Filter::kNearest;
    case GL_LINEAR:
      return Sampler::Filter::kLinear;
    case GL_NEAREST_MIPMAP_NEAREST:
      return Sampler::Filter::kNearestMipmapNearest;
    case GL_LINEAR_MIPMAP_NEAREST:
      return Sampler::Filter::kLinearMipmapNearest;
    case GL_NEAREST_MIPMAP_LINEAR:
      return Sampler::Filter::kNearestMipmapLinear;
    case GL_LINEAR_MIPMAP_LINEAR:
      return Sampler::Filter::kLinearMipmapLinear;
    default:
      return Sampler::Filter::kUnspecified;
  }
}

Sampler::WrapMode TextureWrapMode(int wrap_mode) {
  switch (wrap_mode) {
    case 0:
      return Sampler::WrapMode::kClampToEdge;
    case GL_CLAMP_TO_EDGE:
      return Sampler::WrapMode::kClampToEdge;
    case GL_MIRRORED_REPEAT:
      return Sampler::WrapMode::kMirroredRepeat;
    case GL_REPEAT:
      return Sampler::WrapMode::kRepeat;
    default:
      return Sampler::WrapMode::kUnspecified;
  }
}

}  // namespace redux::tool
