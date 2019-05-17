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

#include "lullaby/systems/render/fpl/mesh.h"
#include "lullaby/systems/render/mesh.h"

#include <vector>

namespace lull {

namespace {

Mesh::MeshImplPtr CreateMesh(const MeshData& src,
                             const fplbase::Attribute* attributes) {
  Mesh::MeshImplPtr mesh(
      new fplbase::Mesh(src.GetVertexBytes(), src.GetNumVertices(),
                        src.GetVertexFormat().GetVertexSize(), attributes,
                        nullptr /* max_position */, nullptr /* min_position */,
                        Mesh::GetFplPrimitiveType(src.GetPrimitiveType())),
      [](const fplbase::Mesh* mesh) { delete mesh; });
  fplbase::Material* mat = nullptr;
  const bool is_32_bit = src.GetIndexSize() == 4;
  mesh->AddIndices(src.GetIndexBytes(), static_cast<int>(src.GetNumIndices()),
                   mat, is_32_bit);
  return mesh;
}

}  // namespace

Mesh::Mesh(MeshImplPtr mesh) : impl_(std::move(mesh)) {
  num_triangles_ =
      impl_ ? static_cast<int>(impl_->CalculateTotalNumberOfIndices() / 3) : 0;
}

Mesh::Mesh(const MeshData& mesh) {
  fplbase::Attribute attributes[kMaxFplAttributeArraySize];
  GetFplAttributes(mesh.GetVertexFormat(), attributes);
  impl_ = CreateMesh(mesh, attributes);
  // TODO: Fix this calculation for different primitive types.
  num_triangles_ = static_cast<int>(mesh.GetNumIndices() / 3);
}

int Mesh::GetNumVertices() const {
  return static_cast<int>(impl_->num_vertices());
}

int Mesh::GetNumTriangles() const { return num_triangles_; }

Aabb Mesh::GetAabb() const {
  return Aabb(impl_->min_position(), impl_->max_position());
}

int Mesh::GetNumBones() const { return static_cast<int>(impl_->num_bones()); }

int Mesh::GetNumShaderBones() const {
  return static_cast<int>(impl_->num_shader_bones());
}

const uint8_t* Mesh::GetBoneParents(int* num) const {
  if (impl_) {
    if (num) {
      *num = static_cast<int>(impl_->num_bones());
    }
    return impl_->bone_parents();
  } else {
    if (num) {
      *num = 0;
    }
    return nullptr;
  }
}

const std::string* Mesh::GetBoneNames(int* num) const {
  if (impl_) {
    if (num) {
      *num = static_cast<int>(impl_->num_bones());
    }
    return impl_->bone_names();
  } else {
    if (num) {
      *num = 0;
    }
    return nullptr;
  }
}

const mathfu::AffineTransform* Mesh::GetDefaultBoneTransformInverses(
    int* num) const {
  if (impl_) {
    if (num) {
      *num = static_cast<int>(impl_->num_bones());
    }
    return impl_->default_bone_transform_inverses();
  } else {
    if (num) {
      *num = 0;
    }
    return nullptr;
  }
}

void Mesh::GatherShaderTransforms(
    const mathfu::AffineTransform* bone_transforms,
    mathfu::AffineTransform* shader_transforms) const {
  impl_->GatherShaderTransforms(bone_transforms, shader_transforms);
}

void Mesh::Render(fplbase::Renderer* renderer, fplbase::BlendMode blend_mode) {
  if (!impl_->IsValid()) {
    return;
  }
  const bool ignore_material = impl_->GetMaterial(0) == nullptr;
  if (ignore_material) {
    renderer->SetBlendMode(blend_mode);
  } else {
    impl_->GetMaterial(0)->set_blend_mode(blend_mode);
  }
  renderer->Render(impl_.get(), ignore_material);
}

// TODO cache fpl attributes for vertex formats.
void Mesh::GetFplAttributes(
    const VertexFormat& format,
    fplbase::Attribute attributes[kMaxFplAttributeArraySize]) {
  // Make sure there's space for the kEND terminator.
  CHECK_LT(format.GetNumAttributes() + 1,
           static_cast<int>(kMaxFplAttributeArraySize));

  size_t offset = 0;
  int texture_index = 0;
  for (size_t i = 0; i < format.GetNumAttributes(); ++i) {
    const VertexAttribute* src = format.GetAttributeAt(i);
    switch (src->usage()) {
      case VertexAttributeUsage_Position:
        if (src->type() == VertexAttributeType_Vec3f) {
          attributes[i] = fplbase::kPosition3f;
        } else if (src->type() == VertexAttributeType_Vec2f) {
          attributes[i] = fplbase::kPosition2f;
        } else {
          LOG(DFATAL) << "kPosition must be a Vec2f or Vec3f.";
          attributes[i] = fplbase::kEND;
        }
        break;
      case VertexAttributeUsage_TexCoord:
        if (src->type() == VertexAttributeType_Vec2f) {
          if (texture_index == 0) {
            attributes[i] = fplbase::kTexCoord2f;
          } else if (texture_index == 1) {
            attributes[i] = fplbase::kTexCoordAlt2f;
          } else {
            LOG(DFATAL) << "Only UV index of 0 or 1 supported.";
          }
          ++texture_index;
        } else if (src->type() == VertexAttributeType_Vec2us) {
          attributes[i] = fplbase::kTexCoord2us;
        } else {
          LOG(DFATAL) << "Unsupported UV format: type " << src->type();
          attributes[i] = fplbase::kEND;
        }
        break;
      case VertexAttributeUsage_Color:
        if (src->type() == VertexAttributeType_Vec4ub) {
          attributes[i] = fplbase::kColor4ub;
        } else {
          LOG(DFATAL) << "kColor must be a Vec4ub.";
          attributes[i] = fplbase::kEND;
        }
        break;
      case VertexAttributeUsage_BoneIndices:
        if (src->type() == VertexAttributeType_Vec4ub) {
          attributes[i] = fplbase::kBoneIndices4ub;
        } else {
          LOG(DFATAL) << "kIndex must be a Vec4ub.";
          attributes[i] = fplbase::kEND;
        }
        break;
      case VertexAttributeUsage_BoneWeights:
        if (src->type() == VertexAttributeType_Vec4ub) {
          attributes[i] = fplbase::kBoneWeights4ub;
        } else {
          LOG(DFATAL) << "kWeight must be a Vec4ub.";
          attributes[i] = fplbase::kEND;
        }
        break;
      case VertexAttributeUsage_Normal:
        if (src->type() == VertexAttributeType_Vec3f) {
          attributes[i] = fplbase::kNormal3f;
        } else {
          LOG(DFATAL) << "kNormal must be a Vec3f.";
          attributes[i] = fplbase::kEND;
        }
        break;
      case VertexAttributeUsage_Tangent:
        if (src->type() == VertexAttributeType_Vec4f) {
          attributes[i] = fplbase::kTangent4f;
        } else {
          LOG(DFATAL) << "kTangent must be a Vec4f.";
          attributes[i] = fplbase::kEND;
        }
        break;
      default:
        LOG(DFATAL) << "Unsupported vertex attribute";
        break;
    }
    // This function requires that the attributes be terminated, so we need
    // to ensure that kEND is appended before this DCHECK.
    attributes[i+1] = fplbase::kEND;
    DCHECK_EQ(offset,
              fplbase::Mesh::AttributeOffset(attributes, attributes[i]));
    offset += VertexFormat::GetAttributeSize(*src);
  }
}

fplbase::Mesh::Primitive Mesh::GetFplPrimitiveType(
    MeshData::PrimitiveType type) {
  switch (type) {
    case MeshData::PrimitiveType::kPoints:
      return fplbase::Mesh::kPoints;
    case MeshData::PrimitiveType::kLines:
      return fplbase::Mesh::kLines;
    case MeshData::PrimitiveType::kTriangles:
      return fplbase::Mesh::kTriangles;
    case MeshData::PrimitiveType::kTriangleFan:
      return fplbase::Mesh::kTriangleFan;
    case MeshData::PrimitiveType::kTriangleStrip:
      return fplbase::Mesh::kTriangleStrip;
    default:
      LOG(ERROR) << "Invalid primitive type; falling back on triangles.";
      return fplbase::Mesh::kTriangles;
  }
}

VertexFormat GetVertexFormat(const MeshPtr& mesh, size_t submesh_index) {
  LOG(ERROR) << "GetVertexFormat() is unsupported.";
  return VertexFormat();
}

bool IsMeshLoaded(const MeshPtr& mesh) {
  return mesh ? mesh->GetNumTriangles() > 0 : false;
}

size_t GetNumSubmeshes(const MeshPtr& mesh) {
  return mesh ? 1 : 0;
}

void SetGpuBuffers(const MeshPtr& mesh, uint32_t vbo, uint32_t vao,
                   uint32_t ibo) {
  LOG(ERROR) << "SetGpuBuffers() is unsupported.";
}

void ReplaceSubmesh(MeshPtr mesh, size_t submesh_index,
                    const MeshData& mesh_data) {
  LOG(ERROR) << "ReplaceSubmesh() is unsupported.";
}

}  // namespace lull
