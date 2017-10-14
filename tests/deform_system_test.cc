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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "lullaby/generated/deform_def_generated.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/systems/deform/deform_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/render/testing/mock_render_system_impl.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/registry.h"
#include "tests/mathfu_matchers.h"
#include "lullaby/generated/transform_def_generated.h"

namespace lull {
namespace {

using ::testing::Contains;
using ::testing::Eq;
using ::testing::FloatNear;
using ::testing::Invoke;
using ::testing::Key;
using ::testing::Not;
using ::testing::_;
using testing::EqualsMathfuMat4;
using testing::NearMathfu;

constexpr float kEpsilon = 1e-5f;
constexpr float kDeformRadius = 2.0f;
const mathfu::vec3 kOrigin(0.0f, 0.0f, 0.0f);

constexpr char kWaypointPathId1[] = "path1";
constexpr char kWaypointPathId2[] = "path2";

class DeformSystemTest : public ::testing::Test {
 protected:
  DeformSystemTest() {
    registry_.Create<Dispatcher>();

    entity_factory_ = registry_.Create<EntityFactory>(&registry_);
    deform_system_ = entity_factory_->CreateSystem<DeformSystem>();
    render_system_ = entity_factory_->CreateSystem<RenderSystem>();
    transform_system_ = entity_factory_->CreateSystem<TransformSystem>();

    mock_render_system_ = render_system_->GetImpl();
    ON_CALL(*mock_render_system_, SetDeformationFunction(_, _))
        .WillByDefault(Invoke([&](Entity e, RenderSystem::Deformation d) {
          deformation_fns_[e] = d;
        }));

    entity_factory_->Initialize();
  }

  ~DeformSystemTest() override = default;

  // Checks that the deform system reports the entity as deformed and that their
  // is a deformation function set on the render system that does not output any
  // errors when called.
  void ExpectDeformedMesh(Entity e) {
    EXPECT_TRUE(deform_system_->IsDeformed(e));
    EXPECT_THAT(deform_system_->GetDeformMode(e), Eq(DeformMode_CylinderBend));
    EXPECT_THAT(deform_system_->GetDeformRadius(e),
                FloatNear(kDeformRadius, kEpsilon));

    ASSERT_THAT(deformation_fns_, Contains(Key(e)));
  }

  // Checks that the deform system reports the entity as undeformed and that
  // there is a deformation function set that either is null or reports an error
  // when called.
  void ExpectUndeformedMesh(Entity e) {
    EXPECT_FALSE(deform_system_->IsDeformed(e));
    EXPECT_THAT(deform_system_->GetDeformMode(e), Eq(DeformMode_None));
    EXPECT_THAT(deform_system_->GetDeformRadius(e), FloatNear(0.0f, kEpsilon));

    ASSERT_THAT(deformation_fns_, Contains(Key(e)));
  }

  // Checks that the entity is not located at the undeformed offset. This does
  // not check that the deformed transform is correct.
  void ExpectDeformedTransform(Entity e, const mathfu::vec3& offset) {
    EXPECT_THAT(
        *transform_system_->GetWorldFromEntityMatrix(e),
        Not(NearMathfu(mathfu::mat4::FromTranslationVector(offset), kEpsilon)));
  }

  // Checks that the entity is located at the undeformed offset.
  void ExpectUndeformedTransform(Entity e, const mathfu::vec3& offset) {
    EXPECT_THAT(
        *transform_system_->GetWorldFromEntityMatrix(e),
        NearMathfu(mathfu::mat4::FromTranslationVector(offset), kEpsilon));
  }

  // Checks that the entity is located at the given offset.
  void ExpectExactTransform(Entity e, const mathfu::vec3& position,
                            const mathfu::vec3& rot_euler) {
    mathfu::mat3 rotation =
        mathfu::quat::FromEulerAngles(rot_euler * kDegreesToRadians).ToMatrix();
    EXPECT_THAT(*transform_system_->GetWorldFromEntityMatrix(e),
                NearMathfu(mathfu::mat4::FromTranslationVector(position) *
                               mathfu::mat4::FromRotationMatrix(rotation),
                           kEpsilon));
  }

  // Checks that |entity| is marked as deformed, regardless of whether it has
  // a deformer.
  void ExpectIsSetAsDeformed(Entity entity) {
    EXPECT_TRUE(deform_system_->IsSetAsDeformed(entity));
  }

  // Checks that the given entity is deformed from previous_pos to expect_pos
  // with expect_rot and set as deformed
  void ExpectWaypointDeformed(Entity deformed, const mathfu::vec3& previous_pos,
                              const mathfu::vec3& expected_pos,
                              const mathfu::vec3& expected_rot) {
    ExpectDeformedTransform(deformed, previous_pos);
    ExpectIsSetAsDeformed(deformed);
    ExpectExactTransform(deformed, expected_pos, expected_rot);
  }

  // Creates an entity for the given deformer with the given translation
  Entity CreateWaypointDeformed(const Entity parent,
                                const mathfu::vec3& translation,
                                string_view path_id, const float size = 0.f) {
    lull::Sqt sqt;
    sqt.translation = translation;
    lull::Aabb aabb(mathfu::vec3(-size / 2.0f), mathfu::vec3(size / 2.0f));
    Entity deformed = entity_factory_->Create();
    transform_system_->Create(deformed, sqt);
    transform_system_->SetAabb(deformed, aabb);
    deform_system_->SetAsDeformed(deformed, path_id);
    transform_system_->AddChild(parent, deformed);
    return deformed;
  }

  Registry registry_;

  EntityFactory* entity_factory_;
  DeformSystem* deform_system_;
  RenderSystem* render_system_;
  TransformSystem* transform_system_;

  RenderSystemImpl* mock_render_system_;
  std::map<Entity, RenderSystem::Deformation> deformation_fns_;
};

TEST_F(DeformSystemTest, DeformerMissingDeformed) {
  Blueprint blueprint;
  {
    TransformDefT transform;
    blueprint.Write(&transform);

    DeformerDefT deformer;
    deformer.horizontal_radius = kDeformRadius;
    blueprint.Write(&deformer);
  }

  EXPECT_CALL(*mock_render_system_, SetDeformationFunction(_, _)).Times(1);
  const Entity deformer = entity_factory_->Create(&blueprint);
  ASSERT_THAT(deformer, Not(Eq(kNullEntity)));

  ExpectDeformedMesh(deformer);
  ExpectUndeformedTransform(deformer, kOrigin);
}

TEST_F(DeformSystemTest, DeformedMissingDeformer) {
  const mathfu::vec3 offset(1.0f, 0.0f, 0.0f);
  Blueprint blueprint;
  {
    TransformDefT transform;
    transform.position = offset;
    blueprint.Write(&transform);

    DeformedDefT deformed;
    blueprint.Write(&deformed);
  }

  EXPECT_CALL(*mock_render_system_, SetDeformationFunction(_, _)).Times(1);

  const Entity undeformed = entity_factory_->Create(&blueprint);
  ASSERT_THAT(undeformed, Not(Eq(kNullEntity)));

  ExpectUndeformedMesh(undeformed);
  ExpectUndeformedTransform(undeformed, offset);
  ExpectIsSetAsDeformed(undeformed);
}

TEST_F(DeformSystemTest, DeformerAndSingleDeformed) {
  const mathfu::vec3 offset(1.0f, 0.0f, 0.0f);
  Blueprint blueprint1;
  {
    TransformDefT transform;
    blueprint1.Write(&transform);

    DeformerDefT deformer;
    deformer.horizontal_radius = kDeformRadius;
    blueprint1.Write(&deformer);
  }

  Blueprint blueprint2;
  {
    TransformDefT transform;
    transform.position = offset;
    blueprint2.Write(&transform);

    DeformedDefT deformed;
    blueprint2.Write(&deformed);
  }

  EXPECT_CALL(*mock_render_system_, SetDeformationFunction(_, _)).Times(2);

  Entity deformer = entity_factory_->Create(&blueprint1);
  Entity deformed = entity_factory_->Create(&blueprint2);

  ASSERT_THAT(deformer, Not(Eq(kNullEntity)));
  ASSERT_THAT(deformed, Not(Eq(kNullEntity)));

  ExpectUndeformedMesh(deformed);
  ExpectUndeformedTransform(deformed, offset);
  ExpectIsSetAsDeformed(deformed);

  transform_system_->AddChild(deformer, deformed);

  ExpectDeformedMesh(deformed);
  ExpectDeformedTransform(deformed, offset);
}

TEST_F(DeformSystemTest, DeformerAndDeformedChain) {
  const mathfu::vec3 offset(1.0f, 0.0f, 0.0f);
  Blueprint blueprint1;
  {
    TransformDefT transform;
    blueprint1.Write(&transform);

    DeformerDefT deformer;
    deformer.horizontal_radius = kDeformRadius;
    blueprint1.Write(&deformer);
  }

  Blueprint blueprint2;
  {
    TransformDefT transform;
    transform.position = offset;
    blueprint2.Write(&transform);

    DeformedDefT deformed;
    blueprint2.Write(&deformed);
  }

  EXPECT_CALL(*mock_render_system_, SetDeformationFunction(_, _)).Times(3);

  Entity deformer = entity_factory_->Create(&blueprint1);
  Entity deformed1 = entity_factory_->Create(&blueprint2);
  Entity deformed2 = entity_factory_->Create(&blueprint2);

  ASSERT_THAT(deformer, Not(Eq(kNullEntity)));
  ASSERT_THAT(deformed1, Not(Eq(kNullEntity)));
  ASSERT_THAT(deformed2, Not(Eq(kNullEntity)));

  ExpectUndeformedMesh(deformed1);
  ExpectUndeformedTransform(deformed1, offset);
  ExpectUndeformedMesh(deformed2);
  ExpectUndeformedTransform(deformed2, offset);

  transform_system_->AddChild(deformed1, deformed2);

  ExpectUndeformedMesh(deformed1);
  ExpectUndeformedTransform(deformed1, offset);
  ExpectUndeformedMesh(deformed2);
  ExpectUndeformedTransform(deformed2, mathfu::vec3(2.0f, 0.0f, 0.0f));

  transform_system_->AddChild(deformer, deformed1);

  ExpectDeformedMesh(deformed1);
  ExpectDeformedTransform(deformed1, offset);
  ExpectDeformedMesh(deformed2);
  ExpectDeformedTransform(deformed2, mathfu::vec3(2.0f, 0.0f, 0.0f));
}

TEST_F(DeformSystemTest, BrokenDeformedChain) {
  const mathfu::vec3 offset(1.0f, 0.0f, 0.0f);
  Blueprint blueprint1;
  {
    TransformDefT transform;
    blueprint1.Write(&transform);

    DeformerDefT deformer;
    deformer.horizontal_radius = kDeformRadius;
    blueprint1.Write(&deformer);
  }

  Blueprint blueprint2;
  {
    TransformDefT transform;
    transform.position = offset;
    blueprint2.Write(&transform);

    DeformedDefT deformed;
    blueprint2.Write(&deformed);
  }

  Blueprint blueprint3;
  {
    TransformDefT transform;
    transform.position = offset;
    blueprint3.Write(&transform);
  }

  EXPECT_CALL(*mock_render_system_, SetDeformationFunction(_, _)).Times(2);

  Entity deformer = entity_factory_->Create(&blueprint1);
  Entity deformed = entity_factory_->Create(&blueprint2);
  Entity undeformed = entity_factory_->Create(&blueprint3);

  ASSERT_THAT(deformer, Not(Eq(kNullEntity)));
  ASSERT_THAT(deformed, Not(Eq(kNullEntity)));
  ASSERT_THAT(undeformed, Not(Eq(kNullEntity)));

  transform_system_->AddChild(undeformed, deformed);

  ExpectUndeformedMesh(deformed);
  ExpectIsSetAsDeformed(deformed);

  ASSERT_THAT(deformation_fns_, Not(Contains(Key(undeformed))));

  transform_system_->AddChild(deformer, undeformed);
  ExpectUndeformedMesh(deformed);
  ExpectIsSetAsDeformed(deformed);
}

TEST_F(DeformSystemTest, DeletedDeformer) {
  const mathfu::vec3 offset(1.0f, 0.0f, 0.0f);
  Blueprint blueprint1;
  {
    TransformDefT transform;
    blueprint1.Write(&transform);

    DeformerDefT deformer;
    deformer.horizontal_radius = kDeformRadius;
    blueprint1.Write(&deformer);
  }

  Blueprint blueprint2;
  {
    TransformDefT transform;
    transform.position = offset;
    blueprint2.Write(&transform);

    DeformedDefT deformed;
    blueprint2.Write(&deformed);
  }

  EXPECT_CALL(*mock_render_system_, SetDeformationFunction(_, _)).Times(3);

  Entity deformer = entity_factory_->Create(&blueprint1);
  Entity deformed = entity_factory_->Create(&blueprint2);

  ASSERT_THAT(deformer, Not(Eq(kNullEntity)));
  ASSERT_THAT(deformed, Not(Eq(kNullEntity)));

  ExpectDeformedMesh(deformer);
  ExpectUndeformedTransform(deformer, kOrigin);
  ExpectUndeformedMesh(deformed);
  ExpectUndeformedTransform(deformed, offset);
  ExpectIsSetAsDeformed(deformed);

  transform_system_->AddChild(deformer, deformed);

  ExpectDeformedMesh(deformer);
  ExpectUndeformedTransform(deformer, kOrigin);
  ExpectDeformedMesh(deformed);
  ExpectDeformedTransform(deformed, offset);

  // We are in the process of destruction and the deformer is destroyed in the
  // deform system prior to being destroyed in the transform system.
  deform_system_->Destroy(deformer);

  ExpectUndeformedMesh(deformer);
  ExpectUndeformedTransform(deformer, kOrigin);
  ExpectUndeformedMesh(deformed);
  ExpectUndeformedTransform(deformed, offset);
  ExpectIsSetAsDeformed(deformed);
}

TEST_F(DeformSystemTest, DeformedInCode) {
  const mathfu::vec3 offset(1.0f, 0.0f, 0.0f);
  Blueprint blueprint;
  {
    TransformDefT transform;
    blueprint.Write(&transform);

    DeformerDefT deformer;
    deformer.horizontal_radius = kDeformRadius;
    blueprint.Write(&deformer);
  }

  EXPECT_CALL(*mock_render_system_, SetDeformationFunction(_, _)).Times(2);

  Entity deformer = entity_factory_->Create(&blueprint);

  lull::Sqt sqt;
  sqt.translation = offset;
  Entity deformed = entity_factory_->Create();
  transform_system_->Create(deformed, sqt);
  deform_system_->SetAsDeformed(deformed);

  ASSERT_THAT(deformer, Not(Eq(kNullEntity)));
  ASSERT_THAT(deformed, Not(Eq(kNullEntity)));

  ExpectUndeformedMesh(deformed);
  ExpectUndeformedTransform(deformed, offset);
  ExpectIsSetAsDeformed(deformed);

  transform_system_->AddChild(deformer, deformed);

  ExpectDeformedMesh(deformed);
  ExpectDeformedTransform(deformed, offset);
}

TEST_F(DeformSystemTest, DeformedSetWorldFromEntityMatrix) {
  const mathfu::vec3 offset(0.5f, 0.0f, 0.0f);
  Blueprint blueprint1;
  {
    TransformDefT transform;
    blueprint1.Write(&transform);

    DeformerDefT deformer;
    deformer.horizontal_radius = kDeformRadius;
    blueprint1.Write(&deformer);
  }

  Blueprint blueprint2;
  {
    TransformDefT transform;
    transform.position = offset;
    blueprint2.Write(&transform);

    DeformedDefT deformed;
    blueprint2.Write(&deformed);
  }

  Entity deformer = entity_factory_->Create(&blueprint1);
  Entity deformed1 = entity_factory_->Create(&blueprint2);
  Entity deformed2 = entity_factory_->Create(&blueprint2);
  transform_system_->AddChild(deformer, deformed1);
  transform_system_->AddChild(deformed1, deformed2);

  const mathfu::mat4 desired_mat =
      *transform_system_->GetWorldFromEntityMatrix(deformed2);

  transform_system_->SetWorldFromEntityMatrix(deformed2, desired_mat);

  const mathfu::mat4 result_mat =
      *transform_system_->GetWorldFromEntityMatrix(deformed2);

  EXPECT_THAT(desired_mat, EqualsMathfuMat4(result_mat));
}

TEST_F(DeformSystemTest, WaypointDeformerTranslation) {
  // Create the deformer with a waypoint mapping that remaps elements upwards
  // on the y axis and very slightly rotates them
  Blueprint blueprint;
  DeformerDefT deformer_def;
  {
    TransformDefT transform;
    blueprint.Write(&transform);

    deformer_def.deform_mode = DeformMode_Waypoint;

    WaypointPathT waypoint_path;
    for (float i = 0.f; i < 4; i++) {
      WaypointT node;
      node.original_position = mathfu::vec3(i, 0.0f, 0.0f);
      node.remapped_position = mathfu::vec3(i, i + 1, 0.0f);
      node.remapped_rotation = mathfu::vec3(0.0f, i + 1, 0.0f);
      waypoint_path.waypoints.push_back(node);
    }
    deformer_def.waypoint_paths.push_back(waypoint_path);
    blueprint.Write(&deformer_def);
  }
  Entity deformer = entity_factory_->Create(&blueprint);

  // Create a series of children of the deformer within the bounds of the
  // mappings and test that they are deformed as expected.
  for (int i = 0; i < 3; i++) {
    // Test a child entity exactly matching one of the waypoint mappings.
    WaypointT waypoint = deformer_def.waypoint_paths.front().waypoints[i];
    Entity deformed = CreateWaypointDeformed(
        deformer, waypoint.original_position, /*path_id=*/"");
    ExpectWaypointDeformed(deformed, waypoint.original_position,
                           waypoint.remapped_position,
                           waypoint.remapped_rotation);

    // Test a child that has to be interpolated.
    mathfu::vec3 original_interpolated_pos =
        waypoint.original_position + mathfu::vec3(0.5f, 0.0f, 0.0f);
    Entity deformed_interpolated = CreateWaypointDeformed(
        deformer, original_interpolated_pos, /*path_id=*/"");
    mathfu::vec3 remapped_interpolated_pos =
        waypoint.remapped_position + mathfu::vec3(0.5f, 0.5f, 0.0f);
    mathfu::vec3 remapped_interpolated_rot_euler =
        waypoint.remapped_rotation + mathfu::vec3(0.0f, 0.5f, 0.0f);
    ExpectWaypointDeformed(deformed_interpolated, original_interpolated_pos,
                           remapped_interpolated_pos,
                           remapped_interpolated_rot_euler);
  }

  // Test that out of bounds children are clamped to the min/max
  mathfu::vec3 below_bounds_pos = mathfu::vec3(-100.0f, 0.0f, 0.0f);
  Entity deformed = CreateWaypointDeformed(deformer, below_bounds_pos,
                                           /*path_id=*/"");
  WaypointT min = deformer_def.waypoint_paths.front().waypoints[0];
  ExpectWaypointDeformed(deformed, below_bounds_pos, min.remapped_position,
                         min.remapped_rotation);
  mathfu::vec3 above_bounds_pos = mathfu::vec3(100.0f, 0.0f, 0.0f);
  deformed = CreateWaypointDeformed(deformer, above_bounds_pos,
                                    /*path_id=*/"");
  WaypointT max = deformer_def.waypoint_paths.front().waypoints.back();
  ExpectWaypointDeformed(deformed, below_bounds_pos, max.remapped_position,
                         max.remapped_rotation);
}

TEST_F(DeformSystemTest, WaypointDeformerTranslationGrandchildren) {
  // Create the deformer with a waypoint mapping that remaps elements upwards
  // on the y axis and very slightly rotates them
  Blueprint blueprint;
  DeformerDefT deformer_def;
  {
    TransformDefT transform;
    blueprint.Write(&transform);

    deformer_def.deform_mode = DeformMode_Waypoint;
    WaypointPathT waypoint_path;
    waypoint_path.path_id = kWaypointPathId1;
    for (float i = 1.f; i < 10; i++) {
      WaypointT node;
      node.original_position = mathfu::vec3(i, 0.0f, 0.0f);
      node.remapped_position = mathfu::vec3(i, i + 1, 0.0f);
      node.remapped_rotation = mathfu::vec3(0.0f, 0.0f, i + 1);
      waypoint_path.waypoints.push_back(node);
    }
    deformer_def.waypoint_paths.push_back(waypoint_path);
    blueprint.Write(&deformer_def);
  }
  Entity deformer = entity_factory_->Create(&blueprint);

  // Test a child entity exactly matching one of the waypoint mappings.
  std::vector<WaypointT> waypoints =
      deformer_def.waypoint_paths.front().waypoints;
  Entity child = CreateWaypointDeformed(
      deformer, waypoints[0].original_position, kWaypointPathId1);
  ExpectWaypointDeformed(child, waypoints[0].original_position,
                         waypoints[0].remapped_position,
                         waypoints[0].remapped_rotation);

  Entity grandchild = CreateWaypointDeformed(
      child, waypoints[1].original_position, kWaypointPathId1);
  // Granchild is set relative to its parent and grandparent, so should expect
  // it to be [3, 0, 0] in the deformer's space and remapped accordingly even
  // though locally it's been set as [2, 0, 0]
  auto expected_pos = mathfu::vec3(3.0f, 4.0f, 0.0f);
  auto expected_rot = mathfu::vec3(0.0f, 0.0f, 4.0f);
  ExpectWaypointDeformed(grandchild, waypoints[1].original_position,
                         expected_pos, expected_rot);

  Entity babby = CreateWaypointDeformed(
      grandchild, waypoints[2].original_position, kWaypointPathId1);
  // Granchild is set relative to its ancestors, so should expect
  // it to be [6, 0, 0] in the deformer's space and remapped accordingly even
  // though locally it's been set as [3, 0, 0]
  expected_pos = mathfu::vec3(6.0f, 7.0f, 0.0f);
  expected_rot = mathfu::vec3(0.0f, 0.0f, 7.0f);
  ExpectWaypointDeformed(babby, waypoints[2].original_position, expected_pos,
                         expected_rot);
}

TEST_F(DeformSystemTest, WaypointDeformerMultiPath) {
  // Create the deformer with a waypoint maps all entities to one of two points,
  // either (1, 0, 0) or (-1, 0, 0).
  Blueprint blueprint;
  DeformerDefT deformer_def;
  {
    TransformDefT transform;
    blueprint.Write(&transform);

    deformer_def.deform_mode = DeformMode_Waypoint;

    WaypointPathT waypoint_path1;
    WaypointPathT waypoint_path2;
    waypoint_path1.path_id = kWaypointPathId1;
    waypoint_path2.path_id = kWaypointPathId2;

    WaypointT node1;
    node1.original_position = mathfu::vec3(0.0f, 0.0f, 0.0f);
    node1.remapped_position = mathfu::vec3(1.0f, 0.0f, 0.0f);
    waypoint_path1.waypoints.push_back(node1);

    node1.remapped_position = mathfu::vec3(-1.0f, 0.0f, 0.0f);
    waypoint_path2.waypoints.push_back(node1);

    WaypointT node2;
    node1.original_position = mathfu::vec3(1.0f, 0.0f, 0.0f);
    node1.remapped_position = mathfu::vec3(1.0f, 0.0f, 0.0f);
    waypoint_path1.waypoints.push_back(node1);

    node2.remapped_position = mathfu::vec3(-1.0f, 0.0f, 0.0f);
    waypoint_path2.waypoints.push_back(node1);

    deformer_def.waypoint_paths.push_back(waypoint_path1);
    deformer_def.waypoint_paths.push_back(waypoint_path2);

    blueprint.Write(&deformer_def);
  }
  Entity deformer = entity_factory_->Create(&blueprint);

  mathfu::vec3 position = mathfu::kZeros3f;
  Entity deformed1 =
      CreateWaypointDeformed(deformer, position, kWaypointPathId1);
  ExpectWaypointDeformed(deformed1, position, mathfu::vec3(1.0f, 0.0f, 0.0f),
                         mathfu::kZeros3f);

  Entity deformed2 =
      CreateWaypointDeformed(deformer, position, kWaypointPathId2);
  ExpectWaypointDeformed(deformed2, position, mathfu::vec3(-1.0f, 0.0f, 0.0f),
                         mathfu::kZeros3f);
}

TEST_F(DeformSystemTest, WaypointDeformerUseAabbAnchor) {
  // Create the deformer with a waypoint mapping that remaps elements upwards
  // on the y axis using aabb anchors.
  Blueprint blueprint;
  DeformerDefT deformer_def;
  {
    TransformDefT transform;
    blueprint.Write(&transform);

    deformer_def.deform_mode = DeformMode_Waypoint;

    WaypointPathT waypoint_path;
    waypoint_path.use_aabb_anchor = true;
    WaypointT node;
    node.remapped_rotation = mathfu::kZeros3f;
    {
      // Left edge of the element will match (0,0).
      node.original_position = mathfu::vec3(0.0f, 0.0f, 0.0f);
      node.remapped_position = mathfu::vec3(0.0f, 1.0f, 0.0f);
      node.original_aabb_anchor = mathfu::vec3(0.0f, 0.5f, 0.5f);
      node.remapped_aabb_anchor = mathfu::vec3(0.0f, 0.5f, 0.5f);
      waypoint_path.waypoints.push_back(node);
    }
    {
      // Right edge of the element will match (4,0).
      node.original_position = mathfu::vec3(4.0f, 0.0f, 0.0f);
      node.remapped_position = mathfu::vec3(4.0f, 2.0f, 0.0f);
      node.original_aabb_anchor = mathfu::vec3(1.0f, 0.5f, 0.5f);
      node.remapped_aabb_anchor = mathfu::vec3(1.0f, 0.5f, 0.5f);
      waypoint_path.waypoints.push_back(node);
    }
    {
      // When the left edge of the element would originally match (4,0),
      // the entity will be remapped so that the right edge is still at 4.
      // The y will become 3 as usual.
      node.original_position = mathfu::vec3(4.0f, 0.0f, 0.0f);
      node.remapped_position = mathfu::vec3(4.0f, 3.0f, 0.0f);
      node.original_aabb_anchor = mathfu::vec3(0.0f, 0.5f, 0.5f);
      node.remapped_aabb_anchor = mathfu::vec3(1.0f, 0.5f, 0.5f);
      waypoint_path.waypoints.push_back(node);
    }
    deformer_def.waypoint_paths.push_back(waypoint_path);
    blueprint.Write(&deformer_def);
  }
  const Entity deformer = entity_factory_->Create(&blueprint);

  // Size 1.0f. Start clamped to the left edge, move to the right edge, then
  // move past the right edge.
  // Original with |y value|     Remapped with |y value|
  //     0   1   2   3   4   5   0   1   2   3   4
  // |---#---|---|---|---#---|   #---|---|---|---#
  //   |-0-|                     |-1-|
  //     |-0-|   '   '   '   '   |-1-|   '   '   '
  //           |-0-|                   |1.5|
  //                 |-0-|   '               |-2-|
  //                   |-0-|                 |2.5|
  //                     |-0-|               |-3-|
  //                       |-0-|             |-3-|
  {
    const mathfu::vec3 original_positions[] = {
        {0.0f, 0.0f, 0.0f}, {0.5f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f},
        {3.5f, 0.0f, 0.0f}, {4.0f, 0.0f, 0.0f}, {4.5f, 0.0f, 0.0f},
        {5.0f, 0.0f, 0.0f},
    };
    const mathfu::vec3 remapped_positions[] = {
        {0.5f, 1.0f, 0.0f}, {0.5f, 1.0f, 0.0f}, {2.0f, 1.5f, 0.0f},
        {3.5f, 2.0f, 0.0f}, {3.5f, 2.5f, 0.0f}, {3.5f, 3.0f, 0.0f},
        {3.5f, 3.0f, 0.0f},
    };

    for (int i = 0; i < 7; ++i) {
      const Entity deformed = CreateWaypointDeformed(
          deformer, original_positions[i], /*path_id=*/"", 1.0f);
      ExpectWaypointDeformed(deformed, original_positions[i],
                             remapped_positions[i], mathfu::kZeros3f);
    }
  }
  // Size 2.0f. Start clamped to the left edge, move to the right edge, then
  // move past the right edge.
  // Original with |y value|         Remapped with |y value|
  //     0   1   2   3   4   5   6   0   1   2   3   4
  // |---#---|---|---|---#---|---|   #---|---|---|---#
  // |---0---|                       |---1---|
  //     |---0---|   '   '   '   '   |---1---|   '   '
  //       |---0---|                   |--1.25-|
  //             |---0---|   '   '           |---2---|
  //                   |---0---|             |--2.75-|
  //                     |---0---|           |---3---|
  //                         |---0---|       |---3---|
  {
    const mathfu::vec3 original_positions[] = {
        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.5f, 0.0f, 0.0f},
        {3.0f, 0.0f, 0.0f}, {4.5f, 0.0f, 0.0f}, {5.0f, 0.0f, 0.0f},
        {6.0f, 0.0f, 0.0f},
    };
    const mathfu::vec3 remapped_positions[] = {
        {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f},  {1.5f, 1.25f, 0.0f},
        {3.0f, 2.0f, 0.0f}, {3.0f, 2.75f, 0.0f}, {3.0f, 3.0f, 0.0f},
        {3.0f, 3.0f, 0.0f},
    };

    for (int i = 0; i < 7; ++i) {
      const Entity deformed = CreateWaypointDeformed(
          deformer, original_positions[i], /*path_id=*/"", 2.0f);
      ExpectWaypointDeformed(deformed, original_positions[i],
                             remapped_positions[i], mathfu::kZeros3f);
    }
  }
}

}  // namespace
}  // namespace lull
