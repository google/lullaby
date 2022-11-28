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

#include <utility>

#include "redux/engines/render/filament/filament_render_engine.h"

namespace redux {

using MeshDataPtr = std::shared_ptr<MeshData>;

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

static filament::VertexBuffer::BufferDescriptor CreateVertexBufferDescriptor(
    const MeshDataPtr& mesh_data) {
  auto* user_data = new MeshDataPtr(mesh_data);
  const auto callback = [](void* buffer, size_t size, void* user) {
    auto* ptr = reinterpret_cast<MeshDataPtr*>(user);
    delete ptr;
  };

  const auto bytes = mesh_data->GetVertexData();
  return filament::VertexBuffer::BufferDescriptor(bytes.data(), bytes.size(),
                                                  callback, user_data);
}

static filament::IndexBuffer::BufferDescriptor CreateIndexBufferDescriptor(
    const MeshDataPtr& mesh_data) {
  auto* user_data = new MeshDataPtr(mesh_data);
  const auto callback = [](void* buffer, size_t size, void* user) {
    auto* ptr = reinterpret_cast<MeshDataPtr*>(user);
    delete ptr;
  };

  const auto bytes = mesh_data->GetIndexData();
  return filament::IndexBuffer::BufferDescriptor(bytes.data(), bytes.size(),
                                                 callback, user_data);
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

static FilamentResourcePtr<filament::VertexBuffer> CreateVertexBuffer(
    filament::Engine* engine, const MeshDataPtr& mesh_data) {
  const uint32_t count = static_cast<uint32_t>(mesh_data->GetNumVertices());
  if (count == 0) {
    return nullptr;
  }

  filament::VertexBuffer::Builder builder;
  builder.vertexCount(count);
  builder.bufferCount(1);

  const VertexFormat& vertex_format = mesh_data->GetVertexFormat();
  const uint8_t vertex_size =
      static_cast<uint8_t>(vertex_format.GetVertexSize());

  uint32_t offset = 0;
  for (size_t index = 0; index < vertex_format.GetNumAttributes(); ++index) {
    const VertexAttribute* attribute = vertex_format.GetAttributeAt(index);

    const auto ftype = ToFilamentAttributeType(attribute->type);
    const auto fusage = ToFilamentAttributeUsage(attribute->usage);
    builder.attribute(fusage, 0, ftype, offset, vertex_size);
    if (ftype == filament::VertexBuffer::AttributeType::UBYTE4) {
      if (fusage == filament::VertexAttribute::COLOR) {
        builder.normalized(fusage);
      }
    }
    offset += VertexFormat::GetAttributeSize(*attribute);
  }

  filament::VertexBuffer* buffer = builder.build(*engine);
  buffer->setBufferAt(*engine, 0, CreateVertexBufferDescriptor(mesh_data));
  return MakeFilamentResource(buffer, engine);
}

static FilamentResourcePtr<filament::IndexBuffer> CreateIndexBuffer(
    filament::Engine* engine, const MeshDataPtr& mesh_data) {
  const uint32_t count = static_cast<uint32_t>(mesh_data->GetNumIndices());
  const size_t vertex_count = mesh_data->GetNumVertices();
  if (count == 0 && vertex_count > 0) {
    // Filament requires an index buffer, so create one here.
    filament::IndexBuffer::Builder builder;
    filament::IndexBuffer::BufferDescriptor desc;

    builder.indexCount(static_cast<uint32_t>(vertex_count));
    if (vertex_count <= std::numeric_limits<uint16_t>::max()) {
      builder.bufferType(filament::IndexBuffer::IndexType::USHORT);
      desc = CreateIndexBufferDescriptorForRange<uint16_t>(vertex_count);
    } else {
      builder.bufferType(filament::IndexBuffer::IndexType::UINT);
      desc = CreateIndexBufferDescriptorForRange<uint32_t>(vertex_count);
    }
    filament::IndexBuffer* buffer = builder.build(*engine);
    buffer->setBuffer(*engine, std::move(desc));
    return MakeFilamentResource(buffer, engine);
  }

  if (count == 0) {
    return nullptr;
  }

  filament::IndexBuffer::Builder builder;
  builder.indexCount(count);
  switch (mesh_data->GetMeshIndexType()) {
    case MeshIndexType::U16:
      builder.bufferType(filament::IndexBuffer::IndexType::USHORT);
      break;
    case MeshIndexType::U32:
      builder.bufferType(filament::IndexBuffer::IndexType::UINT);
      break;
    default:
      LOG(FATAL) << "Unsupported index type.";
      return nullptr;
  }

  filament::IndexBuffer* buffer = builder.build(*engine);
  buffer->setBuffer(*engine, CreateIndexBufferDescriptor(mesh_data));
  return MakeFilamentResource(buffer, engine);
}

static size_t CalculateNumPrimitives(MeshPrimitiveType type,
                                     const size_t count) {
  switch (type) {
    case MeshPrimitiveType::Points:
      return count;
    case MeshPrimitiveType::Lines:
      return count / 2;
    case MeshPrimitiveType::Triangles:
      return count / 3;
    case MeshPrimitiveType::TriangleFan:
      return count - 2;
    case MeshPrimitiveType::TriangleStrip:
      return count - 2;
    default:
      LOG(FATAL) << "Invalid primitive type " << ToString(type);
      return count;
  }
}

static size_t CalculateNumVertices(const VertexFormat& format,
                                   absl::Span<const std::byte> bytes) {
  const size_t size = format.GetVertexSize();
  return size > 0 ? bytes.size() / size : 0;
}

FilamentMesh::FilamentMesh(Registry* registry,
                           const std::shared_ptr<MeshData>& mesh_data) {
  fengine_ = GetFilamentEngine(registry);
  CHECK(fengine_);
  if (mesh_data == nullptr) {
    return;
  }

  fvbuffer_ = CreateVertexBuffer(fengine_, mesh_data);
  fibuffer_ = CreateIndexBuffer(fengine_, mesh_data);

  SubmeshData submesh;
  submesh.vertex_format = mesh_data->GetVertexFormat();
  submesh.index_type = mesh_data->GetMeshIndexType();

  auto parts = mesh_data->GetPartData();
  submeshes_.reserve(parts.size());
  for (const MeshData::PartData& part : parts) {
    submesh.name = part.name;
    submesh.primitive_type = part.primitive_type;
    submesh.range_start = part.start;
    submesh.range_end = part.end;
    submesh.box = part.box;
    submeshes_.push_back(submesh);

    num_primitives_ +=
        CalculateNumPrimitives(part.primitive_type, part.end - part.start);
  }

  num_vertices_ = CalculateNumVertices(mesh_data->GetVertexFormat(),
                                       mesh_data->GetVertexData());
  bounding_box_ = mesh_data->GetBoundingBox();
  NotifyReady();
}

size_t FilamentMesh::GetNumVertices() const { return num_vertices_; }

size_t FilamentMesh::GetNumPrimitives() const { return num_primitives_; }

size_t FilamentMesh::GetNumSubmeshes() const { return submeshes_.size(); }

const FilamentMesh::SubmeshData& FilamentMesh::GetSubmeshData(
    size_t index) const {
  CHECK_LT(index, submeshes_.size());
  return submeshes_[index];
}

Box FilamentMesh::GetBoundingBox() const { return bounding_box_; }

filament::VertexBuffer* FilamentMesh::GetFilamentVertexBuffer() const {
  return fvbuffer_.get();
}

filament::IndexBuffer* FilamentMesh::GetFilamentIndexBuffer() const {
  return fibuffer_.get();
}

}  // namespace redux
