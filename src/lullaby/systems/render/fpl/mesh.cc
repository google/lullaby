/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#include <vector>

namespace lull {

namespace {

template <typename Vertex>
Mesh::MeshImplPtr CreateMesh(const TriangleMesh<Vertex>& src,
                             const fplbase::Attribute* attributes) {
  Mesh::MeshImplPtr mesh(
      new fplbase::Mesh(src.GetVertices().data(),
                        static_cast<int>(src.GetVertices().size()),
                        sizeof(Vertex), attributes, nullptr /* max_position */,
                        nullptr /* min_position */),
      [](const fplbase::Mesh* mesh) { delete mesh; });
  fplbase::Material* mat = nullptr;
  mesh->AddIndices(src.GetIndices().data(),
                   static_cast<int>(src.GetIndices().size()), mat);

  return mesh;
}

Mesh::MeshImplPtr CreateMesh(const MeshData& src,
                             const fplbase::Attribute* attributes) {
  Mesh::MeshImplPtr mesh(
      new fplbase::Mesh(src.GetVertexBytes(), src.GetNumVertices(),
                        src.GetVertexFormat().GetVertexSize(), attributes,
                        nullptr /* max_position */, nullptr /* min_position */,
                        Mesh::GetFplPrimitiveType(src.GetPrimitiveType())),
      [](const fplbase::Mesh* mesh) { delete mesh; });
  fplbase::Material* mat = nullptr;
  const bool is_32_bit = false;
  mesh->AddIndices(src.GetIndexData(), static_cast<int>(src.GetNumIndices()),
                   mat, is_32_bit);
  return mesh;
}

}  // namespace

Mesh::Mesh(MeshImplPtr mesh) : impl_(std::move(mesh)) {
  num_triangles_ =
      impl_ ? static_cast<int>(impl_->CalculateTotalNumberOfIndices() / 3) : 0;
}

// TODO(b/31523782): Remove once pipeline for MeshData is in-place.
Mesh::Mesh(const TriangleMesh<VertexP>& mesh) {
  static const fplbase::Attribute kAttributes[] = {
      fplbase::kPosition3f, fplbase::kEND,
  };
  impl_ = CreateMesh(mesh, kAttributes);
  num_triangles_ = static_cast<int>(mesh.GetIndices().size() / 3);
}

// TODO(b/31523782): Remove once pipeline for MeshData is in-place.
Mesh::Mesh(const TriangleMesh<VertexPT>& mesh) {
  static const fplbase::Attribute kAttributes[] = {
      fplbase::kPosition3f, fplbase::kTexCoord2f, fplbase::kEND,
  };
  impl_ = CreateMesh(mesh, kAttributes);
  num_triangles_ = static_cast<int>(mesh.GetIndices().size() / 3);
}

Mesh::Mesh(const MeshData& mesh) {
  fplbase::Attribute attributes[kMaxFplAttributeArraySize];
  GetFplAttributes(mesh.GetVertexFormat(), attributes);
  impl_ = CreateMesh(mesh, attributes);
  // TODO(b/62088621): Fix this calculation for different primitive types.
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

// TODO(b/30033982) cache fpl attributes for vertex formats.
void Mesh::GetFplAttributes(
    const VertexFormat& format,
    fplbase::Attribute attributes[kMaxFplAttributeArraySize]) {
  // Make sure there's space for the kEND terminator.
  CHECK_LT(format.GetNumAttributes() + 1,
           static_cast<int>(kMaxFplAttributeArraySize));

  for (size_t i = 0; i < format.GetNumAttributes(); ++i) {
    const VertexAttribute& src = format.GetAttributeAt(i);
    switch (src.usage) {
      case VertexAttribute::kPosition:
        DCHECK_EQ(src.index, 0);
        if (src.type == VertexAttribute::kFloat32 && src.count == 3) {
          attributes[i] = fplbase::kPosition3f;
        } else if (src.type == VertexAttribute::kFloat32 && src.count == 2) {
          attributes[i] = fplbase::kPosition2f;
        } else {
          LOG(DFATAL)
              << "kPosition must be a kFloat32 with 2 or 3 elements.";
          attributes[i] = fplbase::kEND;
        }
        break;
      case VertexAttribute::kTexCoord:
        if (src.type == VertexAttribute::kFloat32 && src.count == 2) {
          if (src.index == 0) {
            attributes[i] = fplbase::kTexCoord2f;
          } else if (src.index == 1) {
            attributes[i] = fplbase::kTexCoordAlt2f;
          } else {
            LOG(DFATAL) << "Only UV index of 0 or 1 supported.";
          }
        } else if (src.type == VertexAttribute::kUnsignedInt16 &&
                   src.count == 2) {
          attributes[i] = fplbase::kTexCoord2us;
        } else {
          LOG(DFATAL) << "Unsupported UV format: type " << src.type
                      << ", count " << src.count;
          attributes[i] = fplbase::kEND;
        }
        break;
      case VertexAttribute::kColor:
        DCHECK_EQ(src.index, 0);
        if (src.type == VertexAttribute::kUnsignedInt8 && src.count == 4) {
          attributes[i] = fplbase::kColor4ub;
        } else {
          LOG(DFATAL) << "kColor must be a kUnsignedInt8 with 4 elements.";
          attributes[i] = fplbase::kEND;
        }
        break;
      case VertexAttribute::kIndex:
        DCHECK_EQ(src.index, 0);
        if (src.type == VertexAttribute::kUnsignedInt8 && src.count == 4) {
          attributes[i] = fplbase::kBoneIndices4ub;
        } else {
          LOG(DFATAL) << "kIndex must be a kUnsignedInt8 with 4 elements.";
          attributes[i] = fplbase::kEND;
        }
        break;
      case VertexAttribute::kNormal:
        DCHECK_EQ(src.index, 0);
        if (src.type == VertexAttribute::kFloat32 && src.count == 3) {
          attributes[i] = fplbase::kNormal3f;
        } else {
          LOG(DFATAL) << "kNormal must be a kFloat32 with 3 elements.";
          attributes[i] = fplbase::kEND;
        }
        break;
      default:
        LOG(DFATAL) << "Unsupported vertex attribute";
        break;
    }
    // The Mesh::VertexSize function calculates offset when used in this way.
    // This function also requires that the attributes be terminated, so we need
    // to ensure that kEND is appended before this DCHECK.
    attributes[i+1] = fplbase::kEND;
    DCHECK_EQ(static_cast<size_t>(src.offset),
              fplbase::Mesh::AttributeOffset(attributes, attributes[i]));
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

}  // namespace lull
