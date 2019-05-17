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

#include "lullaby/systems/render/filament/mesh_factory.h"

#include <memory>

#include "filament/VertexBuffer.h"

namespace lull {

static filament::VertexBuffer* CreateVertexBuffer(filament::Engine* engine,
                                                  const MeshData& data) {
  const VertexFormat& vertex_format = data.GetVertexFormat();
  const uint8_t vertex_size =
      static_cast<uint8_t>(vertex_format.GetVertexSize());
  const size_t count = data.GetNumVertices();
  const uint8_t* bytes = data.GetVertexBytes();

  filament::VertexBuffer::Builder builder;
  builder.vertexCount(static_cast<uint32_t>(count));
  builder.bufferCount(1);

  uint32_t offset = 0;
  int tex_coord_count = 0;
  for (size_t index = 0; index < vertex_format.GetNumAttributes(); ++index) {
    const VertexAttribute* attribute = vertex_format.GetAttributeAt(index);
    switch (attribute->usage()) {
      case VertexAttributeUsage_Position: {
        CHECK_EQ(attribute->type(), VertexAttributeType_Vec3f);
        builder.attribute(filament::VertexAttribute::POSITION, 0,
                          filament::VertexBuffer::AttributeType::FLOAT3, offset,
                          vertex_size);
        break;
      }
      case VertexAttributeUsage_Color: {
        CHECK_EQ(attribute->type(), VertexAttributeType_Vec4ub);
        builder.attribute(filament::VertexAttribute::COLOR, 0,
                          filament::VertexBuffer::AttributeType::UBYTE4, offset,
                          vertex_size);
        break;
      }
      case VertexAttributeUsage_Orientation: {
        CHECK_EQ(attribute->type(), VertexAttributeType_Vec4f);
        builder.attribute(filament::VertexAttribute::TANGENTS, 0,
                          filament::VertexBuffer::AttributeType::FLOAT4, offset,
                          vertex_size);
        break;
      }
      case VertexAttributeUsage_TexCoord: {
        CHECK_EQ(attribute->type(), VertexAttributeType_Vec2f);
        CHECK_LT(tex_coord_count, 2);
        builder.attribute(tex_coord_count == 0 ? filament::VertexAttribute::UV0
                                               : filament::VertexAttribute::UV1,
                          0, filament::VertexBuffer::AttributeType::FLOAT2,
                          offset, vertex_size);
        ++tex_coord_count;
        break;
      }
      case VertexAttributeUsage_Tangent: {
        LOG(WARNING) << "Ignoring vertex tangent data";
        break;
      }
      case VertexAttributeUsage_Normal: {
        LOG(WARNING) << "Ignoring vertex normal data";
        break;
      }
      case VertexAttributeUsage_BoneIndices: {
        CHECK_EQ(attribute->type(), VertexAttributeType_Vec4ub);
        builder.attribute(filament::VertexAttribute::BONE_INDICES, 0,
                          filament::VertexBuffer::AttributeType::UBYTE4, offset,
                          vertex_size);
        break;
      }
      case VertexAttributeUsage_BoneWeights: {
        switch (attribute->type()) {
          case VertexAttributeType_Vec4f:
            builder.attribute(filament::VertexAttribute::BONE_WEIGHTS, 0,
                              filament::VertexBuffer::AttributeType::FLOAT4,
                              offset, vertex_size);
            break;
          case VertexAttributeType_Vec4ub:
            builder.attribute(filament::VertexAttribute::BONE_WEIGHTS, 0,
                              filament::VertexBuffer::AttributeType::UBYTE4,
                              offset, vertex_size);
            builder.normalized(filament::VertexAttribute::BONE_WEIGHTS);
            break;
          default:
            LOG(WARNING) << "Ignoring unsupported bone weight "
                            "VertexAttributeType: " << attribute->type();
        }
        break;
      }
      default: {
        LOG(WARNING) << "Unhandled vertex attribute type "
                     << EnumNameVertexAttributeUsage(attribute->usage());
        break;
      }
    }
    offset += VertexFormat::GetAttributeSize(*attribute);
  }

  filament::VertexBuffer* buffer = builder.build(*engine);
  filament::VertexBuffer::BufferDescriptor desc(bytes, count * vertex_size);
  buffer->setBufferAt(*engine, 0, std::move(desc));
  return buffer;
}

static filament::IndexBuffer* CreateIndexBuffer(filament::Engine* engine,
                                                const MeshData& data) {
  const uint8_t* bytes = data.GetIndexBytes();
  const size_t count = data.GetNumIndices();
  const size_t index_size = data.GetIndexSize();

  filament::IndexBuffer::Builder builder;
  builder.indexCount(static_cast<uint32_t>(count));
  switch (data.GetIndexType()) {
    case MeshData::kIndexU16:
      builder.bufferType(filament::IndexBuffer::IndexType::USHORT);
      break;
    case MeshData::kIndexU32:
      builder.bufferType(filament::IndexBuffer::IndexType::UINT);
      break;
    default:
      LOG(DFATAL) << "Unsupported index type " << data.GetIndexType();
      return nullptr;
  }

  filament::IndexBuffer* buffer = builder.build(*engine);
  filament::IndexBuffer::BufferDescriptor desc(bytes, count * index_size);
  buffer->setBuffer(*engine, std::move(desc));
  return buffer;
}

MeshFactoryImpl::MeshFactoryImpl(Registry* registry, filament::Engine* engine)
    : registry_(registry),
      meshes_(ResourceManager<Mesh>::kWeakCachingOnly),
      engine_(engine) {}

MeshPtr MeshFactoryImpl::CreateMesh(MeshData mesh_data) {
  return CreateMesh(&mesh_data, 1);
}

MeshPtr MeshFactoryImpl::CreateMesh(MeshData* mesh_datas, size_t len) {
  auto mesh = std::make_shared<Mesh>(engine_);
  Init(mesh, mesh_datas, len);
  return mesh;
}

MeshPtr MeshFactoryImpl::CreateMesh(HashValue name, MeshData mesh_data) {
  return meshes_.Create(name, [&]() {
    return CreateMesh(&mesh_data, 1);
  });
}

MeshPtr MeshFactoryImpl::CreateMesh(HashValue name,
                                    MeshData* mesh_datas, size_t len) {
  return meshes_.Create(name, [&]() {
    return CreateMesh(mesh_datas, len);
  });
}

void MeshFactoryImpl::Init(MeshPtr mesh, MeshData* mesh_datas, size_t len) {
  std::vector<Mesh::FVertexPtr> vbuffers(len);
  std::vector<Mesh::FIndexPtr> ibuffers(len);

  for (size_t i = 0; i < len; ++i) {
    filament::VertexBuffer* vbuffer =
        CreateVertexBuffer(engine_, mesh_datas[i]);
    filament::IndexBuffer* ibuffer = CreateIndexBuffer(engine_, mesh_datas[i]);
    filament::Engine* engine = engine_;
    Mesh::FVertexPtr vptr(vbuffer, [engine](filament::VertexBuffer* ptr) {
      engine->destroy(ptr);
    });
    Mesh::FIndexPtr iptr(ibuffer, [engine](filament::IndexBuffer* ptr) {
      engine->destroy(ptr);
    });
    vbuffers[i] = std::move(vptr);
    ibuffers[i] = std::move(iptr);
  }

  mesh->Init(vbuffers.data(), ibuffers.data(), mesh_datas, len);
}

MeshPtr MeshFactoryImpl::EmptyMesh() {
  if (!empty_) {
    empty_ = std::make_shared<Mesh>(engine_);
  }
  return empty_;
}

MeshPtr MeshFactoryImpl::GetMesh(HashValue name) const {
  return meshes_.Find(name);
}

void MeshFactoryImpl::CacheMesh(HashValue name, const MeshPtr& mesh) {
  meshes_.Register(name, mesh);
}

void MeshFactoryImpl::ReleaseMesh(HashValue name) { meshes_.Release(name); }

}  // namespace lull
