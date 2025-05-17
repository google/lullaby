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

#include "redux/engines/render/filament/filament_mesh.h"

#include <stdint.h>

#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/types/span.h"
#include "filament/RenderableManager.h"
#include "redux/engines/render/filament/filament_render_engine.h"
#include "redux/engines/render/filament/filament_utils.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/graphics/enums.h"
#include "redux/modules/graphics/graphics_enums_generated.h"
#include "redux/modules/graphics/mesh_data.h"
#include "redux/modules/graphics/mesh_utils.h"
#include "redux/modules/graphics/vertex_attribute.h"
#include "redux/modules/graphics/vertex_format.h"

namespace redux {

using MeshDataPtr = std::shared_ptr<MeshData>;

static filament::RenderableManager::PrimitiveType ToFilamentPrimitiveType(
    MeshPrimitiveType type) {
  switch (type) {
    case MeshPrimitiveType::Triangles:
      return filament::RenderableManager::PrimitiveType::TRIANGLES;
    case MeshPrimitiveType::Points:
      return filament::RenderableManager::PrimitiveType::POINTS;
    case MeshPrimitiveType::Lines:
      return filament::RenderableManager::PrimitiveType::LINES;
    default:
      LOG(FATAL) << "Unsupported primitive type: " << ToString(type);
  }
}

static filament::IndexBuffer::IndexType ToFilamentIndexType(
    MeshIndexType type) {
  switch (type) {
    case MeshIndexType::U16:
      return filament::IndexBuffer::IndexType::USHORT;
    case MeshIndexType::U32:
      return filament::IndexBuffer::IndexType::UINT;
    default:
      LOG(FATAL) << "Unsupported index type: " << ToString(type);
      return filament::IndexBuffer::IndexType::USHORT;
  }
}

static filament::VertexBuffer::AttributeType ToFilamentAttributeType(
    VertexType type) {
  switch (type) {
    case VertexType::Scalar1f:
      return filament::VertexBuffer::AttributeType::FLOAT;
    case VertexType::Vec2f:
      return filament::VertexBuffer::AttributeType::FLOAT2;
    case VertexType::Vec3f:
      return filament::VertexBuffer::AttributeType::FLOAT3;
    case VertexType::Vec4f:
      return filament::VertexBuffer::AttributeType::FLOAT4;
    case VertexType::Vec2us:
      return filament::VertexBuffer::AttributeType::USHORT2;
    case VertexType::Vec4us:
      return filament::VertexBuffer::AttributeType::USHORT4;
    case VertexType::Vec4ub:
      return filament::VertexBuffer::AttributeType::UBYTE4;
    default:
      LOG(FATAL) << "Unsupported vertex type: " << ToString(type);
      return filament::VertexBuffer::AttributeType::FLOAT;
  }
}

static filament::VertexAttribute ToFilamentAttributeUsage(VertexUsage usage) {
  switch (usage) {
    case VertexUsage::Position:
      return filament::VertexAttribute::POSITION;
    case VertexUsage::Orientation:
      return filament::VertexAttribute::TANGENTS;
    case VertexUsage::Color0:
      return filament::VertexAttribute::COLOR;
    case VertexUsage::TexCoord0:
      return filament::VertexAttribute::UV0;
    case VertexUsage::TexCoord1:
      return filament::VertexAttribute::UV1;
    case VertexUsage::BoneIndices:
      return filament::VertexAttribute::BONE_INDICES;
    case VertexUsage::BoneWeights:
      return filament::VertexAttribute::BONE_WEIGHTS;
    default:
      LOG(FATAL) << "Unsupported vertex usage: " << ToString(usage);
      return filament::VertexAttribute::POSITION;
  }
}

// Binds the lifetime of the builder to the buffer descriptor.
static void BindLifetime(filament::backend::BufferDescriptor& desc,
                         const MeshDataPtr& mesh_data) {
  auto* user_data = new MeshDataPtr(mesh_data);
  const auto callback = [](void* buffer, size_t size, void* user) {
    auto* ptr = reinterpret_cast<MeshDataPtr*>(user);
    delete ptr;
  };
  desc.setCallback(callback, user_data);
}

static filament::VertexBuffer::BufferDescriptor CreateVertexBufferDescriptor(
    const MeshDataPtr& mesh_data) {
  const auto bytes = mesh_data->GetVertexData();
  filament::VertexBuffer::BufferDescriptor desc(bytes.data(), bytes.size());
  BindLifetime(desc, mesh_data);
  return desc;
}

static filament::IndexBuffer::BufferDescriptor CreateIndexBufferDescriptor(
    const MeshDataPtr& mesh_data) {
  const auto bytes = mesh_data->GetIndexData();
  filament::VertexBuffer::BufferDescriptor desc(bytes.data(), bytes.size());
  BindLifetime(desc, mesh_data);
  return desc;
}

template <typename T>
static filament::IndexBuffer::BufferDescriptor
CreateIndexBufferDescriptorForRange(size_t count) {
  T* arr = new T[count];
  for (T i = 0; i < static_cast<T>(count); ++i) {
    arr[i] = i;
  }
  const auto callback = [](void* buffer, size_t size, void* user) {
    T* ptr = reinterpret_cast<T*>(user);
    delete[] ptr;
  };
  return filament::IndexBuffer::BufferDescriptor(arr, count * sizeof(T),
                                                 callback, arr);
}

static MeshDataPtr MaybeGenerateOrientations(const MeshDataPtr& mesh_data) {
  bool has_normals = false;
  bool has_orientations = false;

  const VertexFormat& vertex_format = mesh_data->GetVertexFormat();
  for (size_t index = 0; index < vertex_format.GetNumAttributes(); ++index) {
    const VertexAttribute* attribute = vertex_format.GetAttributeAt(index);
    if (attribute->usage == VertexUsage::Normal) {
      has_normals = true;
    } else if (attribute->usage == VertexUsage::Orientation) {
      has_orientations = true;
    }
  }

  if (has_orientations) {
    return nullptr;
  }
  if (!has_normals) {
    return nullptr;
  }
  return std::make_shared<MeshData>(ComputeOrientations(*mesh_data));
}

static void SetupVertexBufferBuilder(filament::VertexBuffer::Builder& builder,
                                     const VertexFormat& vertex_format,
                                     int buffer_index) {
  for (size_t index = 0; index < vertex_format.GetNumAttributes(); ++index) {
    const VertexAttribute* attribute = vertex_format.GetAttributeAt(index);

    // Skip these as they are unsupported by filament. Instead, we need to
    // generate vertex orientations; see MaybeGenerateOrientations above.
    if (attribute->usage == VertexUsage::Normal) {
      continue;
    } else if (attribute->usage == VertexUsage::Tangent) {
      continue;
    }

    const auto ftype = ToFilamentAttributeType(attribute->type);
    const auto fusage = ToFilamentAttributeUsage(attribute->usage);
    const uint32_t offset = vertex_format.GetOffsetOfAttributeAt(index);
    const uint32_t stride = vertex_format.GetStrideOfAttributeAt(index);

    builder.attribute(fusage, buffer_index, ftype, offset, stride);
    if (ftype == filament::VertexBuffer::AttributeType::UBYTE4) {
      if (fusage == filament::VertexAttribute::COLOR) {
        builder.normalized(fusage);
      }
    }
  }
}

static FilamentResourcePtr<filament::VertexBuffer> CreateVertexBuffer(
    filament::Engine* engine, const MeshDataPtr& mesh_data) {
  const uint32_t count = static_cast<uint32_t>(mesh_data->GetNumVertices());
  if (count == 0) {
    return nullptr;
  }

  MeshDataPtr orientations = MaybeGenerateOrientations(mesh_data);

  filament::VertexBuffer::Builder builder;
  builder.vertexCount(count);
  builder.bufferCount(orientations ? 2 : 1);

  SetupVertexBufferBuilder(builder, mesh_data->GetVertexFormat(), 0);
  if (orientations) {
    SetupVertexBufferBuilder(builder, orientations->GetVertexFormat(), 1);
  }

  filament::VertexBuffer* buffer = builder.build(*engine);
  buffer->setBufferAt(*engine, 0, CreateVertexBufferDescriptor(mesh_data));
  if (orientations) {
    buffer->setBufferAt(*engine, 1, CreateVertexBufferDescriptor(orientations));
  }
  return MakeFilamentResource(buffer, engine);
}

static FilamentResourcePtr<filament::IndexBuffer> CreateIndexBuffer(
    filament::Engine* engine, const MeshDataPtr& mesh_data) {
  const uint32_t index_count =
      static_cast<uint32_t>(mesh_data->GetNumIndices());
  const size_t vertex_count = mesh_data->GetNumVertices();

  if (index_count == 0 && vertex_count == 0) {
    return nullptr;
  }

  filament::IndexBuffer::Builder builder;
  filament::IndexBuffer::BufferDescriptor desc;
  if (index_count == 0 && vertex_count > 0) {
    // Filament requires an index buffer, so create one here.
    builder.indexCount(static_cast<uint32_t>(vertex_count));
    if (vertex_count <= std::numeric_limits<uint16_t>::max()) {
      builder.bufferType(filament::IndexBuffer::IndexType::USHORT);
      desc = CreateIndexBufferDescriptorForRange<uint16_t>(vertex_count);
    } else {
      builder.bufferType(filament::IndexBuffer::IndexType::UINT);
      desc = CreateIndexBufferDescriptorForRange<uint32_t>(vertex_count);
    }
  } else {
    builder.indexCount(index_count);
    builder.bufferType(ToFilamentIndexType(mesh_data->GetMeshIndexType()));
    desc = CreateIndexBufferDescriptor(mesh_data);
  }

  filament::IndexBuffer* ibuffer = builder.build(*engine);
  ibuffer->setBuffer(*engine, std::move(desc));
  return MakeFilamentResource(ibuffer, engine);
}

FilamentMesh::FilamentMesh(Registry* registry) {
  fengine_ = GetFilamentEngine(registry);
  CHECK(fengine_);
}

FilamentMesh::FilamentMesh(Registry* registry, absl::Span<MeshData> meshes) {
  fengine_ = GetFilamentEngine(registry);
  CHECK(fengine_);
  CHECK(!meshes.empty());

  const VertexFormat& vertex_format = meshes[0].GetVertexFormat();
  for (size_t index = 0; index < vertex_format.GetNumAttributes(); ++index) {
    const VertexAttribute* attribute = vertex_format.GetAttributeAt(index);

    // Skip these as they are unsupported by filament. Instead, we need to
    // generate vertex orientations; see MaybeGenerateOrientations above.
    if (attribute->usage == VertexUsage::Normal) {
      usages_.push_back(VertexUsage::Orientation);
      continue;
    } else if (attribute->usage == VertexUsage::Tangent) {
      continue;
    }
    usages_.push_back(attribute->usage);
  }

  parts_.reserve(meshes.size());
  for (MeshData& mesh : meshes) {
    MeshDataPtr mesh_ptr = std::make_shared<MeshData>(std::move(mesh));

    PartData& part = parts_.emplace_back();
    part.name = mesh_ptr->GetName();
    part.bounding_box = mesh_ptr->GetBoundingBox();
    part.primitive_type = mesh_ptr->GetPrimitiveType();
    part.vbuffer = CreateVertexBuffer(fengine_, mesh_ptr);
    part.ibuffer = CreateIndexBuffer(fengine_, mesh_ptr);
  }
  parts_.front().name =  HashValue(0);

  NotifyReady();
}

size_t FilamentMesh::GetNumParts() const { return parts_.size(); }

HashValue FilamentMesh::GetPartName(size_t index) const {
  CHECK_LT(index, parts_.size());
  return parts_[index].name;
}

void FilamentMesh::PreparePartRenderable(
    size_t index, filament::RenderableManager::Builder& builder) {
  CHECK_LT(index, parts_.size());
  const auto& part = parts_[index];
  const auto type = ToFilamentPrimitiveType(part.primitive_type);
  builder.boundingBox(ToFilament(part.bounding_box));
  builder.geometry(0, type, part.vbuffer.get(), part.ibuffer.get());
}

absl::Span<const VertexUsage> FilamentMesh::GetVertexUsages() const {
  return usages_;
}

}  // namespace redux
