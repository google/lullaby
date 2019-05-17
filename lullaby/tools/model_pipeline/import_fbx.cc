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

#include <fbxsdk.h>
#include <unordered_set>
#include "lullaby/util/common_types.h"
#include "lullaby/generated/model_pipeline_def_generated.h"
#include "lullaby/tools/common/fbx_base_importer.h"
#include "lullaby/tools/model_pipeline/model.h"
#include "lullaby/tools/model_pipeline/util.h"

namespace lull {
namespace tool {

// Anonymous namespace to avoid collisions with the anim_pipeline FbxImporter.
namespace {

template <class T>
static T ElementFromIndices(const FbxLayerElementTemplate<T>* element,
                            int control_index, int vertex_counter,
                            T default_value = T()) {
  if (!element) {
    return default_value;
  }
  const int index =
      element->GetMappingMode() == FbxGeometryElement::eByControlPoint
          ? control_index
          : vertex_counter;
  const int direct_index =
      element->GetReferenceMode() == FbxGeometryElement::eDirect
          ? index
          : element->GetIndexArray().GetAt(index);
  return element->GetDirectArray().GetAt(direct_index);
}

static mathfu::vec4 Vec4FromFbx(const FbxColor& v) {
  return mathfu::vec4(static_cast<float>(v.mRed), static_cast<float>(v.mGreen),
                      static_cast<float>(v.mBlue),
                      static_cast<float>(v.mAlpha));
}

static mathfu::vec4 Vec4FromFbx(const FbxVector4& v) {
  const FbxDouble* d = v.mData;
  return mathfu::vec4(static_cast<float>(d[0]), static_cast<float>(d[1]),
                      static_cast<float>(d[2]), static_cast<float>(d[3]));
}

static mathfu::vec3 Vec3FromFbx(const FbxVector4& v) {
  const FbxDouble* d = v.mData;
  return mathfu::vec3(static_cast<float>(d[0]), static_cast<float>(d[1]),
                      static_cast<float>(d[2]));
}

static mathfu::vec2 Vec2FromFbx(const FbxVector2& v) {
  const FbxDouble* d = v.mData;
  return mathfu::vec2(static_cast<float>(d[0]), static_cast<float>(d[1]));
}

static mathfu::mat4 Mat4FromFbx(const FbxAMatrix& m) {
  const double* d = m;
  return mathfu::mat4(static_cast<float>(d[0]), static_cast<float>(d[1]),
                      static_cast<float>(d[2]), static_cast<float>(d[3]),
                      static_cast<float>(d[4]), static_cast<float>(d[5]),
                      static_cast<float>(d[6]), static_cast<float>(d[7]),
                      static_cast<float>(d[8]), static_cast<float>(d[9]),
                      static_cast<float>(d[10]), static_cast<float>(d[11]),
                      static_cast<float>(d[12]), static_cast<float>(d[13]),
                      static_cast<float>(d[14]), static_cast<float>(d[15]));
}

// Generates a unit vector (+ handedness) orthogonal to the given normal.
static mathfu::vec4 GenerateTangentFbx(const mathfu::vec3& normal) {
  const mathfu::vec3 axis =
      (std::fabs(mathfu::dot(normal, mathfu::kAxisX3f)) < 0.99f)
          ? mathfu::kAxisX3f
          : mathfu::kAxisY3f;
  return mathfu::vec4(mathfu::normalize(mathfu::cross(normal, axis)), 1.f);
}

class FbxImporter : public FbxBaseImporter {
 public:
  Model Import(const ModelPipelineImportDefT& import_def);

 private:
  // Generates the material for a given mesh.
  void ReadProperty(const FbxProperty& property, Material* material);
  Material GatherMaterial(FbxNode* node, FbxMesh* mesh);

  // Generates the influences for all vertices on a given mesh.
  std::vector<std::vector<Vertex::Influence>> GatherInfluences(
      FbxMesh* mesh, int bone_index, const FbxAMatrix& world_from_model);

  // Builds the skeleton.
  void AddBone(FbxNode* node, FbxNode* parent, const mathfu::mat4& transform);

  // Builds the model.
  void AddMesh(FbxNode* node);
  void BuildDrawable(FbxNode* node, FbxMesh* mesh, int bone_index,
                     const FbxAMatrix& point_transform);

  mathfu::vec2 Vec2FromFbxUv(const FbxVector2& v);

  Model* model_;
  std::unordered_map<FbxNode*, int> node_to_bone_map_;
};

Model FbxImporter::Import(const ModelPipelineImportDefT& import_def) {
  Model model(import_def);

  FbxBaseImporter::Options opts;
  opts.recenter = import_def.recenter;
  opts.axis_system = import_def.axis_system;
  opts.scale_multiplier = import_def.scale;
  opts.cm_per_unit = import_def.cm_per_unit;

  const bool success = LoadScene(import_def.file, opts);
  if (success) {
    model_ = &model;

    ForEachBone([this](FbxNode* node, FbxNode* parent) {
      const FbxAMatrix global_transform = node->EvaluateGlobalTransform();
      const FbxAMatrix default_bone_transform_inverse =
          global_transform.Inverse();
      const mathfu::mat4 transform =
          Mat4FromFbx(default_bone_transform_inverse);
      AddBone(node, parent, transform);
    });
    ForEachMesh([this](FbxNode* node) { AddMesh(node); });

    model.AddImportedPath(import_def.file);
    model_ = nullptr;
  }
  return model;
}

mathfu::vec2 FbxImporter::Vec2FromFbxUv(const FbxVector2& v) {
  const FbxDouble* d = v.mData;
  // FBX UV format has the v-coordinate inverted from OpenGL.  This can be
  // disabled by setting
  // 'flip_texture_coordinates' to false.
  const bool flip_v_coordinate =
      model_->GetImportDef().flip_texture_coordinates;
  return mathfu::vec2(static_cast<float>(d[0]),
                      flip_v_coordinate ? static_cast<float>(1.0 - d[1])
                                        : static_cast<float>(d[1]));
}

void FbxImporter::AddBone(FbxNode* node, FbxNode* parent,
                          const mathfu::mat4& transform) {
  auto iter = node_to_bone_map_.find(parent);
  const int parent_bone_index =
      (iter == node_to_bone_map_.end()) ? -1 : iter->second;

  const Bone bone(node->GetName(), parent_bone_index, transform);
  const int bone_index = model_->AppendBone(bone);
  node_to_bone_map_.emplace(node, bone_index);
}

void FbxImporter::AddMesh(FbxNode* node) {
  // Get the transform to this node from its parent.
  // Note: geometric_transform is applied to each point, but is not inherited
  // by children.
  const FbxVector4 geometric_translation =
      node->GetGeometricTranslation(FbxNode::eSourcePivot);
  const FbxVector4 geometric_rotation =
      node->GetGeometricRotation(FbxNode::eSourcePivot);
  const FbxVector4 geometric_scaling =
      node->GetGeometricScaling(FbxNode::eSourcePivot);
  const FbxAMatrix geometric_transform(geometric_translation,
                                       geometric_rotation, geometric_scaling);
  const FbxAMatrix global_transform = node->EvaluateGlobalTransform();

  // We want the root node to be the identity. Everything in object space
  // is relative to the root.
  FbxAMatrix point_transform = global_transform * geometric_transform;

  // Find the bone for this node.  It must have one, because we checked that
  // it contained a mesh.
  CHECK(node_to_bone_map_.count(node));
  const int bone_index = node_to_bone_map_[node];

  // Gather mesh data for this bone.
  // Note: that there may be more than one mesh attached to a node.
  for (int i = 0; i < node->GetNodeAttributeCount(); ++i) {
    FbxNodeAttribute* attr = node->GetNodeAttributeByIndex(i);
    if (attr == nullptr) {
      continue;
    } else if (attr->GetAttributeType() != FbxNodeAttribute::eMesh) {
      continue;
    }
    FbxMesh* mesh = static_cast<FbxMesh*>(attr);
    BuildDrawable(node, mesh, bone_index, point_transform);
  }
}

void FbxImporter::BuildDrawable(FbxNode* node, FbxMesh* mesh, int bone_index,
                                const FbxAMatrix& point_transform) {
  // Bind the material for the drawable.
  const Material material = GatherMaterial(node, mesh);
  model_->BindDrawable(material);

  // Get references to various vertex elements.
  const FbxVector4* vertices = mesh->GetControlPoints();
  const FbxGeometryElementNormal* normal_element = mesh->GetElementNormal();
  const FbxGeometryElementTangent* tangent_element = mesh->GetElementTangent();
  const FbxGeometryElementVertexColor* color_element =
      mesh->GetElementVertexColor();
  const std::vector<std::vector<Vertex::Influence>> influences =
      GatherInfluences(mesh, bone_index, point_transform);
  const FbxGeometryElementUV* uv_elements[Vertex::kMaxUvs] = {nullptr};
  int num_uvs = mesh->GetElementUVCount();
  if (num_uvs > Vertex::kMaxUvs) {
    LOG(ERROR) << "Ignoring additional uvs.";
    num_uvs = Vertex::kMaxUvs;
  }
  for (int i = 0; i < num_uvs; ++i) {
    uv_elements[i] = mesh->GetElementUV(i);
  }

  // Record which vertex attributes exist for this surface.
  if (vertices) {
    model_->EnableAttribute(Vertex::kAttribBit_Position);
  }
  if (normal_element) {
    model_->EnableAttribute(Vertex::kAttribBit_Normal);
    // Some clients use orientation to communicate normal.
    model_->EnableAttribute(Vertex::kAttribBit_Orientation);
  }
  if (color_element) {
    model_->EnableAttribute(Vertex::kAttribBit_Color0);
  }
  if (tangent_element) {
    model_->EnableAttribute(Vertex::kAttribBit_Tangent);
    model_->EnableAttribute(Vertex::kAttribBit_Orientation);
  }
  for (int i = 0; i < Vertex::kMaxUvs; ++i) {
    if (uv_elements[0]) {
      model_->EnableAttribute(Vertex::kAttribBit_Uv0 + i);
    }
  }
  if (!influences.empty()) {
    model_->EnableAttribute(Vertex::kAttribBit_Influences);
  }

  // Affine matrix only supports multiplication by a point, not a vector.
  // That is, there is no way to ignore the translation (as is required
  // for normals and tangents). So, we create a copy of `transform` that
  // has no translation.
  // http://forums.autodesk.com/t5/fbx-sdk/matrix-vector-multiplication/td-p/4245079
  FbxAMatrix vector_transform = point_transform;
  vector_transform.SetT(FbxVector4(0.0, 0.0, 0.0, 0.0));
  const FbxColor kDefaultColor(1.0, 1.0, 1.0, 1.0);

  int blend_count = 0;
  const FbxBlendShape* blend_deformer = nullptr;

  // For now, save off only the first blend deformer.
  int deformer_count = mesh->GetDeformerCount();
  for (int deformer_loop = 0; deformer_loop < deformer_count; ++deformer_loop) {
    FbxStatus status;
    auto* deformer = mesh->GetDeformer(deformer_loop, &status);
    if (deformer->GetDeformerType() == FbxDeformer::eBlendShape) {
      blend_deformer = static_cast<FbxBlendShape*>(deformer);
      blend_count = blend_deformer->GetBlendShapeChannelCount();
      break;
    }
  }

  int vertex_counter = 0;
  const int num_polys = mesh->GetPolygonCount();
  for (int poly_index = 0; poly_index < num_polys; ++poly_index) {
    const int num_verts = mesh->GetPolygonSize(poly_index);
    CHECK_EQ(num_verts, 3)
        << "Triangulate should have forced all polys to be of size 3.";
    for (int vert_index = 0; vert_index < num_verts; ++vert_index) {
      // Get the control index for this poly, vert combination.
      const int control_index = mesh->GetPolygonVertex(poly_index, vert_index);

      // Depending on the FBX format, normals and UVs are indexed either
      // by control point or by polygon-vertex.
      const FbxVector4 vertex_fbx = vertices[control_index];
      const FbxVector4 normal_fbx =
          ElementFromIndices(normal_element, control_index, vertex_counter);
      const FbxVector4 tangent_fbx =
          ElementFromIndices(tangent_element, control_index, vertex_counter);
      const FbxVector2 uv_fbx =
          ElementFromIndices(uv_elements[0], control_index, vertex_counter);
      const FbxVector2 uv_alt_fbx =
          ElementFromIndices(uv_elements[1], control_index, vertex_counter);
      const FbxColor color_fbx = ElementFromIndices(
          color_element, control_index, vertex_counter, kDefaultColor);

      // Output this vertex.
      Vertex vertex;
      vertex.position = Vec3FromFbx(point_transform.MultT(vertex_fbx));
      vertex.normal =
          Vec3FromFbx(vector_transform.MultT(normal_fbx)).Normalized();
      vertex.tangent = mathfu::vec4(
          Vec3FromFbx(vector_transform.MultT(tangent_fbx)).Normalized(),
          static_cast<float>(tangent_fbx[3]));
      if (std::isnan(vertex.tangent.x)) {
        // Conjure an arbitrary tangent so we can encode normal via orientation.
        vertex.tangent = GenerateTangentFbx(vertex.normal);
      }
      vertex.orientation = CalculateOrientation(vertex.normal, vertex.tangent);
      vertex.color0 = Vec4FromFbx(color_fbx);
      // Note that the v-axis is flipped between FBX UVs and the desired UVs.
      vertex.uv0 = Vec2FromFbxUv(uv_fbx);
      vertex.uv1 = Vec2FromFbxUv(uv_alt_fbx);
      vertex.influences = influences[control_index];
      if (vertex.influences.empty()) {
        vertex.influences.emplace_back(bone_index, 1.f);
      }

      // Go through each blend shape and pull the same polygon.
      for (int blend_index = 0; blend_index < blend_count; ++blend_index) {
        auto* blend_shapes = blend_deformer->GetBlendShapeChannel(blend_index);
        int target_shape_count = blend_shapes->GetTargetShapeCount();
        for (int target_shape_loop = 0; target_shape_loop < target_shape_count;
             ++target_shape_loop) {
          auto* blend_shape = blend_shapes->GetTargetShape(target_shape_loop);

          // For blends, we will only concern ourselves with pos/norm/tangent.
          const FbxVector4* vertices = blend_shape->GetControlPoints();
          const FbxGeometryElementNormal* normal_element =
              blend_shape->GetElementNormal();
          const FbxGeometryElementTangent* tangent_element =
              blend_shape->GetElementTangent();

          // Depending on the FBX format, normals and UVs are indexed either
          // by control point or by polygon-vertex.
          const FbxVector4 vertex_fbx = vertices[control_index];
          const FbxVector4 normal_fbx =
              ElementFromIndices(normal_element, control_index, vertex_counter);
          const FbxVector4 tangent_fbx = ElementFromIndices(
              tangent_element, control_index, vertex_counter);

          Vertex::Blend blend;
          blend.name = blend_shape->GetName();
          blend.position = Vec3FromFbx(point_transform.MultT(vertex_fbx));
          blend.normal =
              Vec3FromFbx(vector_transform.MultT(normal_fbx)).Normalized();
          blend.tangent = mathfu::vec4(
              Vec3FromFbx(vector_transform.MultT(tangent_fbx)).Normalized(),
              static_cast<float>(tangent_fbx[3]));
          blend.orientation =
              CalculateOrientation(vertex.normal, vertex.tangent);
          vertex.blends.push_back(blend);
        }
      }

      model_->AddVertex(vertex);

      // Control points are listed in order of poly + vertex.
      ++vertex_counter;
    }
  }
}

std::vector<std::vector<Vertex::Influence>> FbxImporter::GatherInfluences(
    FbxMesh* mesh, int bone_index, const FbxAMatrix& point_transform) {
  const unsigned int point_count = mesh->GetControlPointsCount();
  std::vector<std::vector<Vertex::Influence>> influences(point_count);

  // Each cluster stores a mapping from a bone to all the vertices it
  // influences.  This generates an inverse mapping from each point to all
  // the bones influencing it.
  const int skin_count = mesh->GetDeformerCount(FbxDeformer::eSkin);
  for (int skin_index = 0; skin_index != skin_count; ++skin_index) {
    FbxSkin* skin = static_cast<FbxSkin*>(
        mesh->GetDeformer(skin_index, FbxDeformer::eSkin));
    const int cluster_count = skin->GetClusterCount();
    for (int cluster_index = 0; cluster_index != cluster_count;
         ++cluster_index) {
      FbxCluster* cluster = skin->GetCluster(cluster_index);
      FbxNode* link_node = cluster->GetLink();

      // Get the bone index from the node pointer.
      CHECK(node_to_bone_map_.count(link_node));
      const int bone_index = node_to_bone_map_[link_node];

      // The "global initial transform of the geometry node that contains the
      // link node", meaning the global initial transform of the node that
      // contains the mesh or the world-from-mesh matrix.
      FbxAMatrix fbx_world_from_mesh;
      cluster->GetTransformMatrix(fbx_world_from_mesh);

      // The "global initial transform of the link node", meaning the global
      // initial transform of the link itself. Because the link is the bone,
      // this is the world-from-bone matrix.
      FbxAMatrix fbx_world_from_bone;
      cluster->GetTransformLinkMatrix(fbx_world_from_bone);

      // Combining these two gives the bone-from-mesh matrix, which is often
      // referred to as the "inverse bind pose" since it undoes the "binding" of
      // the mesh to the skin.
      FbxAMatrix fbx_bone_from_mesh =
          fbx_world_from_bone.Inverse() * fbx_world_from_mesh;

      // Optimize skinning by combining the inverse bind matrix and the un-bake
      // matrix into the resulting model's inverse bind matrix.
      model_->SetInverseBindTransform(
          bone_index,
          Mat4FromFbx(fbx_bone_from_mesh * point_transform.Inverse()));

      // We currently only support normalized weights.  Both eNormalize and
      // eTotalOne can be treated as normalized, because we renormalize
      // weights after extraction.
      const FbxCluster::ELinkMode link_mode = cluster->GetLinkMode();
      if (link_mode != FbxCluster::eNormalize &&
          link_mode != FbxCluster::eTotalOne) {
        LOG(ERROR) << "Unknown link mode: " << link_mode;
      }

      // Assign bone weights to all cluster influences.
      const int influence_count = cluster->GetControlPointIndicesCount();
      const int* point_indices = cluster->GetControlPointIndices();
      const double* weights = cluster->GetControlPointWeights();
      for (int influence_index = 0; influence_index != influence_count;
           ++influence_index) {
        const int point_index = point_indices[influence_index];
        assert(static_cast<unsigned int>(point_index) < point_count);
        const float weight = static_cast<float>(weights[influence_index]);
        influences[point_index].emplace_back(bone_index, weight);
      }
    }
  }
  return influences;
}

static MaterialTextureUsage ConvertUsage(const std::string& name,
                                         FbxTexture::ETextureUse use) {
  if (name.find("Diffuse") != std::string::npos) {
    return MaterialTextureUsage_BaseColor;
  } else if (name.find("Normal") != std::string::npos) {
    return MaterialTextureUsage_Normal;
  } else if (name.find("Bump") != std::string::npos) {
    return MaterialTextureUsage_Bump;
  } else if (name.find("Specular") != std::string::npos) {
    return MaterialTextureUsage_Specular;
  } else if (name.find("Gloss") != std::string::npos) {
    return MaterialTextureUsage_Metallic;
  } else if (name.find("Light") != std::string::npos) {
    return MaterialTextureUsage_Light;
  } else if (name.find("Shadow") != std::string::npos) {
    return MaterialTextureUsage_Shadow;
  } else if (name.find("Reflection") != std::string::npos) {
    return MaterialTextureUsage_Reflection;
  } else if (name.find("TEX_color_map") != std::string::npos) {
    return MaterialTextureUsage_BaseColor;
  } else if (name.find("TEX_normal_map") != std::string::npos) {
    return MaterialTextureUsage_Normal;
  } else if (name.find("TEX_emissive_map") != std::string::npos) {
    return MaterialTextureUsage_Emissive;
  } else if (name.find("TEX_ao_map") != std::string::npos) {
    return MaterialTextureUsage_Occlusion;
  } else if (name.find("TEX_roughness_map") != std::string::npos) {
    return MaterialTextureUsage_Roughness;
  } else if (name.find("TEX_metallic_map") != std::string::npos) {
    return MaterialTextureUsage_Metallic;
  } else if (name.find("TEX_roughness_map") != std::string::npos) {
    return MaterialTextureUsage_Roughness;
  } else if (name.find("TEX_brdf_lut") != std::string::npos) {
    return MaterialTextureUsage_BrdfLookupTable;
  } else if (name.find("TEX_global_diffuse_cube") != std::string::npos) {
    return MaterialTextureUsage_DiffuseEnvironment;
  } else if (name.find("TEX_global_specular_cube") != std::string::npos) {
    return MaterialTextureUsage_SpecularEnvironment;
  }

  switch (use) {
    case FbxTexture::eStandard:
      return MaterialTextureUsage_BaseColor;
    case FbxTexture::eShadowMap:
      return MaterialTextureUsage_Shadow;
    case FbxTexture::eLightMap:
      return MaterialTextureUsage_Light;
    case FbxTexture::eSphericalReflectionMap:
      return MaterialTextureUsage_Reflection;
    case FbxTexture::eSphereReflectionMap:
      return MaterialTextureUsage_Reflection;
    case FbxTexture::eBumpNormalMap:
      return MaterialTextureUsage_Normal;
    default:
      LOG(ERROR) << "Unknown texture usage: " << use;
      return MaterialTextureUsage_BaseColor;
  }
}

static TextureWrap ConvertWrapMode(FbxTexture::EWrapMode mode) {
  switch (mode) {
    case FbxTexture::eClamp:
      return TextureWrap_ClampToEdge;
    case FbxTexture::eRepeat:
      return TextureWrap_Repeat;
    default:
      LOG(ERROR) << "Unknown wrap mode: " << mode;
      return TextureWrap_Repeat;
  }
}

void FbxImporter::ReadProperty(const FbxProperty& property,
                               Material* material) {
  const std::string name = property.GetName().Buffer();
  if (property.GetSrcObjectCount<FbxTexture>() > 0) {
    for (int i = 0; i < property.GetSrcObjectCount<FbxFileTexture>(); ++i) {
      FbxFileTexture* texture = property.GetSrcObject<FbxFileTexture>(i);

      TextureInfo info;
      info.usages = {ConvertUsage(name, texture->GetTextureUse())};
      info.wrap_s = ConvertWrapMode(texture->GetWrapModeU());
      info.wrap_t = ConvertWrapMode(texture->GetWrapModeV());
      info.premultiply_alpha = texture->GetPremultiplyAlpha();

      const std::string filename = texture->GetRelativeFileName();
      material->textures[filename] = info;
    }
    for (int i = 0; i < property.GetSrcObjectCount<FbxLayeredTexture>(); ++i) {
      FbxLayeredTexture* layered_texture =
          property.GetSrcObject<FbxLayeredTexture>(i);
      // Inspect the layers to see if it is composite or if it just boils down
      // to one normal texture; if it's the latter, pretend it's the only one.
      FbxFileTexture* single_texture = nullptr;
      // Whether our current result would be composed from multiple input
      // textures.  Used to disambiguate single_texture==nullptr (false: current
      // result is empty/black; true: current result is composite).
      bool composite = false;
      const int layer_count =
          layered_texture->GetSrcObjectCount<FbxFileTexture>();
      for (int layer_index = 0; layer_index < layer_count; ++layer_index) {
        FbxLayeredTexture::EBlendMode layer_blend_mode =
            FbxLayeredTexture::eBlendModeCount;
        if (!layered_texture->GetTextureBlendMode(layer_index,
                                                  layer_blend_mode)) {
          // Invalid if we can't query.
          single_texture = nullptr;
          break;
        }

        double alpha = 0.0;
        if (!layered_texture->GetTextureAlpha(layer_index, alpha)) {
          // Invalid if we can't query.
          single_texture = nullptr;
          break;
        }

        if (alpha == 0.0) {
          // Skip layers that are completely transparent since they don't affect
          // the composite texture.
          continue;
        }

        if ((layer_blend_mode == FbxLayeredTexture::eAdditive ||
             layer_blend_mode == FbxLayeredTexture::eOver ||
             layer_blend_mode == FbxLayeredTexture::eTranslucent) &&
            single_texture == nullptr && !composite) {
          // An 'additive', 'over', or 'translucent' layer, when adding to
          // (or blending against) nothing, resolves to a single texture.
          single_texture =
              layered_texture->GetSrcObject<FbxFileTexture>(layer_index);
        } else if (layer_blend_mode == FbxLayeredTexture::eNormal) {
          // A 'normal' layer just replaces what's beneath it.
          single_texture =
              layered_texture->GetSrcObject<FbxFileTexture>(layer_index);
          composite = false;
        } else {
          // Otherwise, our status as of this level of the evaluation is a
          // composite of multiple textures.
          composite = true;
          single_texture = nullptr;
        }
      }

      if (single_texture) {
        TextureInfo info;
        info.usages = {ConvertUsage(name, single_texture->GetTextureUse())};
        info.wrap_s = ConvertWrapMode(single_texture->GetWrapModeU());
        info.wrap_t = ConvertWrapMode(single_texture->GetWrapModeV());
        info.premultiply_alpha = single_texture->GetPremultiplyAlpha();
        std::string filename = single_texture->GetRelativeFileName();
        material->textures[filename] = info;
      } else {
        LOG_ONCE(ERROR) << "Unsupported Layered Texture configuration.";
      }
    }
    if (property.GetSrcObjectCount<FbxProceduralTexture>() > 0) {
      LOG_ONCE(ERROR) << "Procedural textures not supported.";
    }
  } else {
    const FbxDataType type = property.GetPropertyDataType();
    if (FbxBoolDT == type) {
      material->properties[name] = static_cast<bool>(property.Get<FbxBool>());
    } else if (FbxIntDT == type || FbxEnumDT == type) {
      material->properties[name] = static_cast<int>(property.Get<FbxInt>());
    } else if (FbxFloatDT == type) {
      material->properties[name] = static_cast<float>(property.Get<FbxFloat>());
    } else if (FbxDoubleDT == type) {
      material->properties[name] =
          static_cast<double>(property.Get<FbxDouble>());
    } else if (FbxStringDT == type || FbxUrlDT == type ||
               FbxXRefUrlDT == type) {
      material->properties[name] =
          std::string(property.Get<FbxString>().Buffer());
    } else if (FbxDouble2DT == type) {
      auto value = property.Get<FbxDouble2>();
      material->properties[name] = mathfu::vec2(value[0], value[1]);
    } else if (FbxDouble3DT == type || FbxColor3DT == type) {
      auto value = property.Get<FbxDouble3>();
      material->properties[name] = mathfu::vec3(value[0], value[1], value[2]);
    } else if (FbxDouble4DT == type || FbxColor4DT == type) {
      auto value = property.Get<FbxDouble4>();
      material->properties[name] =
          mathfu::vec4(value[0], value[1], value[2], value[3]);
    } else if (FbxDouble4x4DT == type) {
      auto value = property.Get<FbxDouble4x4>();
      material->properties[name] =
          mathfu::mat4(value[0][0], value[0][1], value[0][2], value[0][3],
                       value[1][0], value[1][1], value[1][2], value[1][3],
                       value[2][0], value[2][1], value[2][2], value[2][3],
                       value[3][0], value[3][1], value[3][2], value[3][3]);
    } else if (FbxCompoundDT == type) {
      // TODO: Figure out what this property does; filtering it
      // out of error spam because stingray assets seem to have lots of them.
    } else if (FbxReferenceDT == type) {
      // According to the documentation, FbxReference is an internal property.
    } else {
      LOG_ONCE(ERROR) << "Unsupported property type: " << type.GetName();
    }
  }
}

Material FbxImporter::GatherMaterial(FbxNode* node, FbxMesh* mesh) {
  Material material;

  FbxLayerElementArrayTemplate<int>* material_indices;
  const bool valid_indices = mesh->GetMaterialIndices(&material_indices);
  if (!valid_indices) {
    return material;
  }

  for (int i = 0; i < material_indices->GetCount(); ++i) {
    const int material_index = (*material_indices)[i];
    const FbxSurfaceMaterial* fbx_material = node->GetMaterial(material_index);
    if (fbx_material == nullptr) {
      continue;
    }

    const char* name = fbx_material->GetName();
    if (name) {
      material.name = std::string(name);
      material.properties["Name"] = std::string(name);
    }

    for (FbxProperty property = fbx_material->GetFirstProperty();
         property.IsValid();
         property = fbx_material->GetNextProperty(property)) {
      ReadProperty(property, &material);
    }
  }
  return material;
}

}  // namespace

Model ImportFbx(const ModelPipelineImportDefT& import_def) {
  FbxImporter importer;
  return importer.Import(import_def);
}

}  // namespace tool
}  // namespace lull
