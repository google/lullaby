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

#include <fbxsdk.h>
#include <unordered_set>
#include "lullaby/util/common_types.h"
#include "lullaby/generated/model_pipeline_def_generated.h"
#include "tools/model_pipeline/model.h"

namespace lull {
namespace tool {

static FbxAxisSystem AxisSystemToFbxAxisSystem(AxisSystem system) {
  int up = FbxAxisSystem::eXAxis;
  int front = FbxAxisSystem::eParityEven;
  int coord = FbxAxisSystem::eRightHanded;
  switch (system) {
    case AxisSystem_Unspecified:
      break;
    case AxisSystem_XUp_YFront_ZLeft:
      up = FbxAxisSystem::eXAxis;
      front = FbxAxisSystem::eParityEven;
      coord = FbxAxisSystem::eRightHanded;
      break;
    case AxisSystem_XUp_YFront_ZRight:
      up = FbxAxisSystem::eXAxis;
      front = FbxAxisSystem::eParityEven;
      coord = FbxAxisSystem::eLeftHanded;
      break;
    case AxisSystem_XUp_YBack_ZLeft:
      up = FbxAxisSystem::eXAxis;
      front = FbxAxisSystem::eParityOdd;
      coord = FbxAxisSystem::eRightHanded;
      break;
    case AxisSystem_XUp_YBack_ZRight:
      up = FbxAxisSystem::eXAxis;
      front = FbxAxisSystem::eParityOdd;
      coord = FbxAxisSystem::eLeftHanded;
      break;
    case AxisSystem_XUp_ZFront_YLeft:
      up = FbxAxisSystem::eXAxis;
      front = -FbxAxisSystem::eParityEven;
      coord = FbxAxisSystem::eRightHanded;
      break;
    case AxisSystem_XUp_ZFront_YRight:
      up = FbxAxisSystem::eXAxis;
      front = -FbxAxisSystem::eParityEven;
      coord = FbxAxisSystem::eLeftHanded;
      break;
    case AxisSystem_XUp_ZBack_YLeft:
      up = FbxAxisSystem::eXAxis;
      front = -FbxAxisSystem::eParityOdd;
      coord = FbxAxisSystem::eRightHanded;
      break;
    case AxisSystem_XUp_ZBack_YRight:
      up = FbxAxisSystem::eXAxis;
      front = -FbxAxisSystem::eParityOdd;
      coord = FbxAxisSystem::eLeftHanded;
      break;
    case AxisSystem_YUp_XFront_ZLeft:
      up = FbxAxisSystem::eYAxis;
      front = FbxAxisSystem::eParityEven;
      coord = FbxAxisSystem::eRightHanded;
      break;
    case AxisSystem_YUp_XFront_ZRight:
      up = FbxAxisSystem::eYAxis;
      front = FbxAxisSystem::eParityEven;
      coord = FbxAxisSystem::eLeftHanded;
      break;
    case AxisSystem_YUp_XBack_ZLeft:
      up = FbxAxisSystem::eYAxis;
      front = FbxAxisSystem::eParityOdd;
      coord = FbxAxisSystem::eRightHanded;
      break;
    case AxisSystem_YUp_XBack_ZRight:
      up = FbxAxisSystem::eYAxis;
      front = FbxAxisSystem::eParityOdd;
      coord = FbxAxisSystem::eLeftHanded;
      break;
    case AxisSystem_YUp_ZFront_XLeft:
      up = FbxAxisSystem::eYAxis;
      front = -FbxAxisSystem::eParityEven;
      coord = FbxAxisSystem::eRightHanded;
      break;
    case AxisSystem_YUp_ZFront_XRight:
      up = FbxAxisSystem::eYAxis;
      front = -FbxAxisSystem::eParityEven;
      coord = FbxAxisSystem::eLeftHanded;
      break;
    case AxisSystem_YUp_ZBack_XLeft:
      up = FbxAxisSystem::eYAxis;
      front = -FbxAxisSystem::eParityOdd;
      coord = FbxAxisSystem::eRightHanded;
      break;
    case AxisSystem_YUp_ZBack_XRight:
      up = FbxAxisSystem::eYAxis;
      front = -FbxAxisSystem::eParityOdd;
      coord = FbxAxisSystem::eLeftHanded;
      break;
    case AxisSystem_ZUp_XFront_YLeft:
      up = FbxAxisSystem::eZAxis;
      front = FbxAxisSystem::eParityEven;
      coord = FbxAxisSystem::eRightHanded;
      break;
    case AxisSystem_ZUp_XFront_YRight:
      up = FbxAxisSystem::eZAxis;
      front = FbxAxisSystem::eParityEven;
      coord = FbxAxisSystem::eLeftHanded;
      break;
    case AxisSystem_ZUp_XBack_YLeft:
      up = FbxAxisSystem::eZAxis;
      front = FbxAxisSystem::eParityOdd;
      coord = FbxAxisSystem::eRightHanded;
      break;
    case AxisSystem_ZUp_XBack_YRight:
      up = FbxAxisSystem::eZAxis;
      front = FbxAxisSystem::eParityOdd;
      coord = FbxAxisSystem::eLeftHanded;
      break;
    case AxisSystem_ZUp_YFront_XLeft:
      up = FbxAxisSystem::eZAxis;
      front = -FbxAxisSystem::eParityEven;
      coord = FbxAxisSystem::eRightHanded;
      break;
    case AxisSystem_ZUp_YFront_XRight:
      up = FbxAxisSystem::eZAxis;
      front = -FbxAxisSystem::eParityEven;
      coord = FbxAxisSystem::eLeftHanded;
      break;
    case AxisSystem_ZUp_YBack_XLeft:
      up = FbxAxisSystem::eZAxis;
      front = -FbxAxisSystem::eParityOdd;
      coord = FbxAxisSystem::eRightHanded;
      break;
    case AxisSystem_ZUp_YBack_XRight:
      up = FbxAxisSystem::eZAxis;
      front = -FbxAxisSystem::eParityOdd;
      coord = FbxAxisSystem::eLeftHanded;
      break;
    default:
      break;
  }
  return FbxAxisSystem(static_cast<FbxAxisSystem::EUpVector>(up),
                       static_cast<FbxAxisSystem::EFrontVector>(front),
                       static_cast<FbxAxisSystem::ECoordSystem>(coord));
}

static void ConvertFbxScale(float distance_unit, FbxScene* scene) {
  if (distance_unit <= 0.0f) {
    return;
  }
  const FbxSystemUnit import_unit = scene->GetGlobalSettings().GetSystemUnit();
  const FbxSystemUnit export_unit(distance_unit);
  if (import_unit != export_unit) {
    export_unit.ConvertScene(scene);
  }
}

void ConvertFbxAxes(AxisSystem axis_system, FbxScene* scene) {
  if (axis_system == AxisSystem_Unspecified) {
    return;
  }

  const FbxAxisSystem import_axes = scene->GetGlobalSettings().GetAxisSystem();
  const FbxAxisSystem export_axes = AxisSystemToFbxAxisSystem(axis_system);
  if (import_axes != export_axes) {
    export_axes.ConvertScene(scene);
  }

  // The FBX SDK has a bug. After an axis conversion, the prerotation is not
  // propagated to the PreRotation property. We propagate the values manually.
  // Note that we only propagate to the children of the root, since those are
  // the only nodes affected by axis conversion.
  FbxNode* root = scene->GetRootNode();
  for (int i = 0; i < root->GetChildCount(); i++) {
    FbxNode* node = root->GetChild(i);
    node->PreRotation.Set(node->GetPreRotation(FbxNode::eSourcePivot));
  }
}

static bool NodeHasMesh(FbxNode* node) {
  if (node->GetMesh() != nullptr) {
    return true;
  }
  for (int i = 0; i < node->GetChildCount(); i++) {
    if (NodeHasMesh(node->GetChild(i))) {
      return true;
    }
  }
  return false;
}

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

// FBX UV format has the v-coordinate inverted from OpenGL.
static mathfu::vec2 Vec2FromFbxUv(const FbxVector2& v) {
  const FbxDouble* d = v.mData;
  return mathfu::vec2(static_cast<float>(d[0]), static_cast<float>(1.0 - d[1]));
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

static mathfu::vec4 CalculateOrientation(const mathfu::vec3& normal,
                                         const mathfu::vec4& tangent) {
  const mathfu::vec3 n = normal.Normalized();
  const mathfu::vec3 t = tangent.xyz().Normalized();
  const mathfu::vec3 b = mathfu::vec3::CrossProduct(n, t).Normalized();
  const mathfu::mat3 m(t.x, t.y, t.z, b.x, b.y, b.z, n.x, n.y, n.z);
  mathfu::quat q = mathfu::quat::FromMatrix(m).Normalized();
  // Align the sign bit of the orientation scalar to our handedness.
  if (signbit(tangent.w) != signbit(q.scalar())) {
    q = mathfu::quat(-q.scalar(), -q.vector());
  }
  return mathfu::vec4(q.vector(), q.scalar());
}

class FbxImporter {
 public:
  FbxImporter();
  ~FbxImporter();

  Model Import(const ModelPipelineImportDefT& import_def);

 private:
  // Prepares the fbx scene geometry for exporting.
  void ConvertGeometry();
  void ConvertGeometryRecursive(FbxNode* node);

  // Generates the bode_to_node_map_.
  bool MarkBoneNodesRecursive(FbxNode* node);
  int AddBoneForNode(FbxNode* node, int parent_bone_index);

  // Generates the material for a given mesh.
  void ReadProperty(const FbxProperty& property, Material* material);
  Material GatherMaterial(FbxNode* node, FbxMesh* mesh);

  // Generates the influences for all vertices on a given mesh.
  std::vector<std::vector<Vertex::Influence>> GatherInfluences(FbxMesh* mesh,
                                                               int bone_index);

  // Builds the model.
  void BuildBonesRecursive(FbxNode* node, int parent_bone_index);
  void BuildMeshRecursive(FbxNode* node, FbxNode* parent_node);
  void BuildDrawable(FbxNode* node, FbxMesh* mesh, int bone_index,
                     const FbxAMatrix& point_transform);

  FbxManager* manager_;
  FbxScene* scene_;
  Model* model_;
  std::unordered_map<FbxNode*, int> node_to_bone_map_;
};

FbxImporter::FbxImporter() {
  manager_ = FbxManager::Create();
  if (manager_ == nullptr) {
    LOG(ERROR) << "Unable to create FBX manager";
    return;
  }

  // Initialize with standard IO settings.
  FbxIOSettings* ios = FbxIOSettings::Create(manager_, IOSROOT);
  manager_->SetIOSettings(ios);

  // Create an FBX scene. This object holds most objects imported/exported
  // from/to files.
  scene_ = FbxScene::Create(manager_, "My Scene");
  if (scene_ == nullptr) {
    LOG(ERROR) << "Unable to create FBX scene.";
    return;
  }
}

FbxImporter::~FbxImporter() {
  if (manager_ != nullptr) {
    manager_->Destroy();
  }
}

Model FbxImporter::Import(const ModelPipelineImportDefT& import_def) {
  Model model(import_def);
  if (manager_ == nullptr || scene_ == nullptr) {
    return model;
  }

  model_ = &model;

  // Create the importer and initialize with the file.
  ::FbxImporter* importer = ::FbxImporter::Create(manager_, "");

  const std::string& filename = model_->GetImportDef().file;
  const bool init_success =
      importer->Initialize(filename.c_str(), -1, manager_->GetIOSettings());
  const FbxStatus init_status = importer->GetStatus();

  // Check the SDK and pipeline versions.
  int sdk_major = 0;
  int sdk_minor = 0;
  int sdk_revision = 0;
  FbxManager::GetFileFormatVersion(sdk_major, sdk_minor, sdk_revision);

  // Report version information.
  LOG(INFO) << "SDK version " << sdk_major << "." << sdk_minor << "."
            << sdk_revision;

  int file_major = 0;
  int file_minor = 0;
  int file_revision = 0;
  importer->GetFileVersion(file_major, file_minor, file_revision);
  LOG(INFO) << "File version " << file_major << "." << file_minor << "."
            << file_revision;

  // Exit on load error.
  if (!init_success) {
    LOG(ERROR) << "Failed loading: " << init_status.GetErrorString();
    return model;
  }

  // Import the scene.
  const bool import_success = importer->Import(scene_);
  const FbxStatus import_status = importer->GetStatus();

  // Clean-up temporaries.
  importer->Destroy();

  // Exit if the import failed.
  if (!import_success) {
    LOG(ERROR) << "Failed import: " << import_status.GetErrorString();
    return model;
  }

  // Ensure the correct distance unit and axis system are being used.
  ConvertFbxScale(model_->GetImportDef().scale, scene_);
  ConvertFbxAxes(model_->GetImportDef().axis_system, scene_);
  ConvertGeometry();
  ConvertGeometryRecursive(scene_->GetRootNode());

  FbxNode* root_node = scene_->GetRootNode();

  // First pass: determine which nodes are to be treated as bones.
  // We skip the root node so it's not included in the bone hierarchy.
  const int child_count = root_node->GetChildCount();
  for (int child_index = 0; child_index != child_count; ++child_index) {
    FbxNode* child_node = root_node->GetChild(child_index);
    MarkBoneNodesRecursive(child_node);
  }

  // Second pass: add bones.
  // We skip the root node so it's not included in the bone hierarchy.
  for (int child_index = 0; child_index != child_count; ++child_index) {
    FbxNode* child_node = root_node->GetChild(child_index);
    BuildBonesRecursive(child_node, -1);
  }

  // Final pass: Traverse the scene and output one surface per mesh.
  BuildMeshRecursive(root_node, root_node);
  model_ = nullptr;
  return model;
}

bool FbxImporter::MarkBoneNodesRecursive(FbxNode* node) {
  // We need a bone for this node if it has a skeleton attribute or a mesh.
  bool valid_bone = (node->GetSkeleton() || node->GetMesh());

  // We also need a bone for this node if it has any such child bones.
  const int child_count = node->GetChildCount();
  for (int child_index = 0; child_index != child_count; ++child_index) {
    FbxNode* child_node = node->GetChild(child_index);
    if (MarkBoneNodesRecursive(child_node)) {
      valid_bone = true;
    }
  }

  // Flag the node as a bone.
  if (valid_bone) {
    const int bone_index = Bone::kInvalidBoneIndex;
    node_to_bone_map_.emplace(node, bone_index);
  }
  return valid_bone;
}

void FbxImporter::BuildBonesRecursive(FbxNode* node, int parent_bone_index) {
  const int bone_index = AddBoneForNode(node, parent_bone_index);
  if (bone_index >= 0) {
    const int child_count = node->GetChildCount();
    for (int child_index = 0; child_index != child_count; ++child_index) {
      FbxNode* child_node = node->GetChild(child_index);
      BuildBonesRecursive(child_node, bone_index);
    }
  }
}

int FbxImporter::AddBoneForNode(FbxNode* node, int parent_bone_index) {
  // The node is a valid bone if it was marked by MarkBoneNodesRecursive.
  auto iter = node_to_bone_map_.find(node);
  if (iter == node_to_bone_map_.end()) {
    return Bone::kInvalidBoneIndex;
  }

  // Add the bone entry.
  const FbxAMatrix global_transform = node->EvaluateGlobalTransform();
  const FbxAMatrix default_bone_transform_inverse = global_transform.Inverse();

  const char* name = node->GetName();
  const Bone bone(name, parent_bone_index,
                  Mat4FromFbx(default_bone_transform_inverse));
  const int bone_index = model_->AppendBone(bone);
  iter->second = bone_index;
  return bone_index;
}

void FbxImporter::ConvertGeometry() {
  FbxGeometryConverter geo_converter(manager_);
  if (model_->GetImportDef().recenter) {
    geo_converter.RecenterSceneToWorldCenter(scene_, 0.0);
  }
  geo_converter.SplitMeshesPerMaterial(scene_, true);
  geo_converter.Triangulate(scene_, true);
}

void FbxImporter::ConvertGeometryRecursive(FbxNode* node) {
  if (node == nullptr) {
    return;
  }

  for (int i = 0; i < node->GetNodeAttributeCount(); ++i) {
    FbxNodeAttribute* attr = node->GetNodeAttributeByIndex(i);
    if (attr == nullptr) {
      continue;
    } else if (attr->GetAttributeType() != FbxNodeAttribute::eMesh) {
      continue;
    }
    FbxMesh* mesh = static_cast<FbxMesh*>(attr);
    mesh->GenerateNormals();
    mesh->GenerateTangentsData(0);
  }

  // Recursively traverse each node in the scene
  for (int i = 0; i < node->GetChildCount(); i++) {
    ConvertGeometryRecursive(node->GetChild(i));
  }
}

void FbxImporter::BuildMeshRecursive(FbxNode* node, FbxNode* parent_node) {
  // We're only interested in mesh nodes. If a node and all nodes under it
  // have no meshes, we early out.
  if (node == nullptr) {
    return;
  } else if (!NodeHasMesh(node)) {
    return;
  }

  // The root node cannot have a transform applied to it, so we do not export it
  // as a bone.
  if (node != scene_->GetRootNode()) {
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
    const FbxAMatrix parent_global_transform =
        parent_node->EvaluateGlobalTransform();

    // We want the root node to be the identity. Everything in object space
    // is relative to the root.
    FbxAMatrix default_bone_transform_inverse = global_transform.Inverse();
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

  for (int i = 0; i < node->GetChildCount(); i++) {
    BuildMeshRecursive(node->GetChild(i), node);
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
      GatherInfluences(mesh, bone_index);
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
      vertex.orientation = CalculateOrientation(vertex.normal, vertex.tangent);
      vertex.color0 = Vec4FromFbx(color_fbx);
      // Note that the v-axis is flipped between FBX UVs and the desired UVs.
      vertex.uv0 = Vec2FromFbxUv(uv_fbx);
      vertex.uv1 = Vec2FromFbxUv(uv_alt_fbx);
      vertex.influences = influences[control_index];
      if (vertex.influences.empty()) {
        vertex.influences.emplace_back(bone_index, 1.f);
      }
      model_->AddVertex(vertex);

      // Control points are listed in order of poly + vertex.
      ++vertex_counter;
    }
  }
}

std::vector<std::vector<Vertex::Influence>> FbxImporter::GatherInfluences(
    FbxMesh* mesh, int bone_index) {
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
    return MaterialTextureUsage_Diffuse;
  } else if (name.find("Normal") != std::string::npos) {
    return MaterialTextureUsage_Normal;
  } else if (name.find("Bump") != std::string::npos) {
    return MaterialTextureUsage_Bump;
  } else if (name.find("Specular") != std::string::npos) {
    return MaterialTextureUsage_Specular;
  } else if (name.find("Gloss") != std::string::npos) {
    return MaterialTextureUsage_Gloss;
  } else if (name.find("Light") != std::string::npos) {
    return MaterialTextureUsage_Light;
  } else if (name.find("Shadow") != std::string::npos) {
    return MaterialTextureUsage_Shadow;
  } else if (name.find("Reflection") != std::string::npos) {
    return MaterialTextureUsage_Reflection;
  }

  switch (use) {
    case FbxTexture::eStandard:
      return MaterialTextureUsage_Diffuse;
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
      return MaterialTextureUsage_Diffuse;
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
      info.usage = ConvertUsage(name, texture->GetTextureUse());
      info.wrap_s = ConvertWrapMode(texture->GetWrapModeU());
      info.wrap_t = ConvertWrapMode(texture->GetWrapModeV());
      info.premultiply_alpha = texture->GetPremultiplyAlpha();

      const std::string filename = texture->GetFileName();
      material->textures[filename] = info;
    }
    if (property.GetSrcObjectCount<FbxLayeredTexture>() > 0) {
      LOG(ERROR) << "Layered textures not supported.";
    }
    if (property.GetSrcObjectCount<FbxProceduralTexture>() > 0) {
      LOG(ERROR) << "Procedural textures not supported.";
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
    } else {
      LOG(ERROR) << "Unsupported property type: " << type.GetName();
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

Model ImportFbx(const ModelPipelineImportDefT& import_def) {
  FbxImporter importer;
  return importer.Import(import_def);
}

}  // namespace tool
}  // namespace lull
