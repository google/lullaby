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

#include "lullaby/contrib/shader_clip/shader_clip_system.h"

#include "gtest/gtest.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/render/testing/mock_render_system_impl.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/generated/shader_clip_def_generated.h"
#include "lullaby/tests/mathfu_matchers.h"
#include "lullaby/generated/transform_def_generated.h"

namespace lull {
namespace {

using ::testing::_;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Not;
using testing::NearMathfu;
using testing::NearMathfuQuat;
using testing::NearMathfuVec3;

constexpr char kMinInClipRegionSpace[] = "min_in_clip_region_space";
constexpr char kMaxInClipRegionSpace[] = "max_in_clip_region_space";
constexpr char kClipRegionFromModelSpaceMatrix[] =
    "clip_region_from_model_space_matrix";
constexpr float kEpsilon = 0.0001f;

class ClipSystemTest : public ::testing::Test {
 public:
  void SetUp() override {
    registry_.Register(MakeUnique<Dispatcher>());

    auto* entity_factory = registry_.Create<EntityFactory>(&registry_);
    entity_factory->CreateSystem<ShaderClipSystem>();
    auto* render_system = entity_factory->CreateSystem<RenderSystem>();
    entity_factory->CreateSystem<TransformSystem>();

    mock_render_system_ = render_system->GetImpl();

    ON_CALL(*mock_render_system_, SetUniform(_, _, _, _, _)).WillByDefault(
        Invoke([this](Entity e, const char* name, const float* data,
                      int dimension, int count) {
          this->set_uniforms_[e][name].assign(data,
                                              data + dimension * count);
        }));

    entity_factory->Initialize();
  }

 protected:
  // Lullaby registry, which owns the various Lullaby systems used in this test.
  Registry registry_;
  // Pointer to the mock render system, which receives calls from the clip
  // system. Owned by the registry.
  NiceMockRenderSystem* mock_render_system_ = nullptr;
  // Set uniforms in the render system for each entity.
  std::unordered_map<Entity,
      std::unordered_map<std::string, std::vector<float>>> set_uniforms_;
};

// Tests the basic clip system functions with AddTarget, Destroy.
TEST_F(ClipSystemTest, AddTargetDestroy) {
  auto* entity_factory = registry_.Get<EntityFactory>();
  auto* shader_clip_system = registry_.Get<ShaderClipSystem>();
  auto* transform_system = registry_.Get<TransformSystem>();

  Blueprint clip_blueprint;
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(1.f, 2.f, 3.f);
    transform.rotation = mathfu::vec3(90.f, 0.f, 0.f);
    transform.scale = mathfu::vec3(1.f, 2.f, 3.f);
    ShaderClipDefT clip;
    clip.min_in_clip_region_space = mathfu::vec3(-4.f, -5.f, -6.f);
    clip.max_in_clip_region_space = mathfu::vec3(7.f, 8.f, 9.f);
    clip_blueprint.Write(&transform);
    clip_blueprint.Write(&clip);
  }
  Blueprint target_blueprint;
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(11.f, 12.f, 13.f);
    transform.rotation = mathfu::vec3(90.f, 0.f, 0.f);
    transform.scale = mathfu::vec3(10.f, 20.f, 30.f);
    target_blueprint.Write(&transform);
  }

  const Entity region = entity_factory->Create(&clip_blueprint);
  const Entity target = entity_factory->Create(&target_blueprint);
  transform_system->AddChild(region, target);
  shader_clip_system->AddTarget(region, target);

  // The clipdef region should not get uniforms, whereas the target does.
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  auto iter = set_uniforms_.find(target);
  ASSERT_THAT(iter, Not(Eq(set_uniforms_.end())));
  auto& target_uniforms = iter->second;

  // The min and max uniforms should match the region's ClipDef.
  {
    auto iter = target_uniforms.find(kMinInClipRegionSpace);
    ASSERT_THAT(iter, Not(Eq(target_uniforms.end())));
    const mathfu::vec3 min(&iter->second[0]);
    EXPECT_THAT(min, NearMathfuVec3(mathfu::vec3(-4.f, -5.f, -6.f), kEpsilon));
  }
  {
    auto iter = target_uniforms.find(kMaxInClipRegionSpace);
    ASSERT_THAT(iter, Not(Eq(target_uniforms.end())));
    const mathfu::vec3 max(&iter->second[0]);
    EXPECT_THAT(max, NearMathfuVec3(mathfu::vec3(7.f, 8.f, 9.f), kEpsilon));
  }

  shader_clip_system->Update();
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  // The region_from_model uniform should match the target's transform.
  {
    auto iter = target_uniforms.find(kClipRegionFromModelSpaceMatrix);
    ASSERT_THAT(iter, Not(Eq(target_uniforms.end())));
    const mathfu::mat4 region_from_model(&iter->second[0]);
    const Sqt sqt = CalculateSqtFromMatrix(region_from_model);
    const mathfu::quat quat = mathfu::quat::FromEulerAngles(
        mathfu::vec3(90.f, 0.f, 0.f) * kDegreesToRadians);
    EXPECT_THAT(sqt.translation,
                NearMathfuVec3(mathfu::vec3(11.f, 12.f, 13.f), kEpsilon));
    EXPECT_THAT(sqt.rotation, NearMathfuQuat(quat, kEpsilon));
    EXPECT_THAT(sqt.scale,
                NearMathfuVec3(mathfu::vec3(10.f, 20.f, 30.f), kEpsilon));
  }

  entity_factory->Destroy(region);
  set_uniforms_.clear();
  shader_clip_system->Update();

  // After destroy there should be no more uniforms set.
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(target), set_uniforms_.end());
}

// Tests the basic clip system functions with a descendent TargetDef, Destroy.
TEST_F(ClipSystemTest, TargetDefDestroy) {
  auto* entity_factory = registry_.Get<EntityFactory>();
  auto* shader_clip_system = registry_.Get<ShaderClipSystem>();
  auto* transform_system = registry_.Get<TransformSystem>();

  BlueprintTree clip_blueprint;
  BlueprintTree* parent_blueprint = clip_blueprint.NewChild();
  BlueprintTree* target_blueprint = parent_blueprint->NewChild();
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(1.f, 2.f, 3.f);
    transform.rotation = mathfu::vec3(90.f, 0.f, 0.f);
    transform.scale = mathfu::vec3(1.f, 2.f, 3.f);
    ShaderClipDefT clip;
    clip.min_in_clip_region_space = mathfu::vec3(-4.f, -5.f, -6.f);
    clip.max_in_clip_region_space = mathfu::vec3(7.f, 8.f, 9.f);
    clip_blueprint.Write(&transform);
    clip_blueprint.Write(&clip);
  }
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(-2.f, -4.f, -6.f);
    parent_blueprint->Write(&transform);
  }
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(11.f, 12.f, 13.f);
    transform.rotation = mathfu::vec3(90.f, 0.f, 0.f);
    transform.scale = mathfu::vec3(10.f, 20.f, 30.f);
    ShaderClipTargetDefT target;
    target_blueprint->Write(&transform);
    target_blueprint->Write(&target);
  }

  const Entity region = entity_factory->Create(&clip_blueprint);
  // The target is the grandchild.
  const std::vector<Entity>* parents = transform_system->GetChildren(region);
  ASSERT_EQ(false, parents->empty());
  const Entity& parent = parents->at(0);
  const std::vector<Entity>* children = transform_system->GetChildren(parent);
  ASSERT_EQ(false, children->empty());
  const Entity& target = children->at(0);

  // The region and parent should not get uniforms, whereas the target does.
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(parent), set_uniforms_.end());
  auto iter = set_uniforms_.find(target);
  ASSERT_THAT(iter, Not(Eq(set_uniforms_.end())));
  auto& target_uniforms = iter->second;

  // The min and max uniforms should match the region's ClipDef.
  {
    auto iter = target_uniforms.find(kMinInClipRegionSpace);
    ASSERT_THAT(iter, Not(Eq(target_uniforms.end())));
    const mathfu::vec3 min(&iter->second[0]);
    EXPECT_THAT(min, NearMathfuVec3(mathfu::vec3(-4.f, -5.f, -6.f), kEpsilon));
  }
  {
    auto iter = target_uniforms.find(kMaxInClipRegionSpace);
    ASSERT_THAT(iter, Not(Eq(target_uniforms.end())));
    const mathfu::vec3 max(&iter->second[0]);
    EXPECT_THAT(max, NearMathfuVec3(mathfu::vec3(7.f, 8.f, 9.f), kEpsilon));
  }

  shader_clip_system->Update();
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(parent), set_uniforms_.end());
  // The parent slightly modifies the child's location.
  {
    auto iter = target_uniforms.find(kClipRegionFromModelSpaceMatrix);
    ASSERT_THAT(iter, Not(Eq(target_uniforms.end())));
    const mathfu::mat4 region_from_model(&iter->second[0]);
    const Sqt sqt = CalculateSqtFromMatrix(region_from_model);
    const mathfu::quat quat = mathfu::quat::FromEulerAngles(
        mathfu::vec3(90.f, 0.f, 0.f) * kDegreesToRadians);
    EXPECT_THAT(sqt.translation,
                NearMathfuVec3(mathfu::vec3(9.f, 8.f, 7.f), kEpsilon));
    EXPECT_THAT(sqt.rotation, NearMathfuQuat(quat, kEpsilon));
    EXPECT_THAT(sqt.scale,
                NearMathfuVec3(mathfu::vec3(10.f, 20.f, 30.f), kEpsilon));
  }

  entity_factory->Destroy(parent);
  set_uniforms_.clear();
  shader_clip_system->Update();

  // After destroy there should be no more uniforms set.
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(parent), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(target), set_uniforms_.end());
}

// Tests that creating an orphaned TargetDef first, then adding it to a region
// will connect the target to the region.
TEST_F(ClipSystemTest, OrphanedTargetDef) {
  auto* entity_factory = registry_.Get<EntityFactory>();
  auto* shader_clip_system = registry_.Get<ShaderClipSystem>();
  auto* transform_system = registry_.Get<TransformSystem>();

  BlueprintTree clip_blueprint;
  BlueprintTree* parent_blueprint = clip_blueprint.NewChild();
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(1.f, 2.f, 3.f);
    transform.rotation = mathfu::vec3(90.f, 0.f, 0.f);
    transform.scale = mathfu::vec3(1.f, 2.f, 3.f);
    ShaderClipDefT clip;
    clip.min_in_clip_region_space = mathfu::vec3(-4.f, -5.f, -6.f);
    clip.max_in_clip_region_space = mathfu::vec3(7.f, 8.f, 9.f);
    clip_blueprint.Write(&transform);
    clip_blueprint.Write(&clip);
  }
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(-2.f, -4.f, -6.f);
    parent_blueprint->Write(&transform);
  }
  Blueprint target_blueprint;
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(11.f, 12.f, 13.f);
    transform.rotation = mathfu::vec3(90.f, 0.f, 0.f);
    transform.scale = mathfu::vec3(10.f, 20.f, 30.f);
    ShaderClipTargetDefT target;
    target_blueprint.Write(&transform);
    target_blueprint.Write(&target);
  }

  const Entity region = entity_factory->Create(&clip_blueprint);
  // The target will be attached to the parent, but start it off orphaned.
  const std::vector<Entity>* parents = transform_system->GetChildren(region);
  ASSERT_EQ(false, parents->empty());
  const Entity& parent = parents->at(0);
  const Entity target = entity_factory->Create(&target_blueprint);

  // The region and parent should not get uniforms, whereas the target does.
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(parent), set_uniforms_.end());
  auto iter = set_uniforms_.find(target);
  ASSERT_THAT(iter, Not(Eq(set_uniforms_.end())));
  auto& inactive_target_uniforms = iter->second;
  // The inactive target should have its uniforms set to passive values.
  {
    auto iter = inactive_target_uniforms.find(
        kClipRegionFromModelSpaceMatrix);
    const mathfu::mat4 region_from_model(&iter->second[0]);
    iter = inactive_target_uniforms.find(kMinInClipRegionSpace);
    const mathfu::vec3 min(&iter->second[0]);
    iter = inactive_target_uniforms.find(kMaxInClipRegionSpace);
    const mathfu::vec3 max(&iter->second[0]);
    EXPECT_THAT(region_from_model, NearMathfu(mathfu::mat4(0.f), kEpsilon));
    EXPECT_THAT(min, NearMathfuVec3(-mathfu::kOnes3f, kEpsilon));
    EXPECT_THAT(max, NearMathfuVec3(mathfu::kOnes3f, kEpsilon));
  }

  // No uniforms should get updated.
  set_uniforms_.clear();
  shader_clip_system->Update();
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(parent), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(target), set_uniforms_.end());

  // Add the target and it should now get uniforms.
  transform_system->AddChild(parent, target);
  shader_clip_system->Update();
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(parent), set_uniforms_.end());
  iter = set_uniforms_.find(target);
  ASSERT_THAT(iter, Not(Eq(set_uniforms_.end())));
  auto& target_uniforms = iter->second;
  // The min and max uniforms should match the region's ClipDef.
  {
    auto iter = target_uniforms.find(kMinInClipRegionSpace);
    ASSERT_THAT(iter, Not(Eq(target_uniforms.end())));
    const mathfu::vec3 min(&iter->second[0]);
    EXPECT_THAT(min, NearMathfuVec3(mathfu::vec3(-4.f, -5.f, -6.f), kEpsilon));
  }
  {
    auto iter = target_uniforms.find(kMaxInClipRegionSpace);
    ASSERT_THAT(iter, Not(Eq(target_uniforms.end())));
    const mathfu::vec3 max(&iter->second[0]);
    EXPECT_THAT(max, NearMathfuVec3(mathfu::vec3(7.f, 8.f, 9.f), kEpsilon));
  }
  // The parent slightly modifies the child's location.
  {
    auto iter = target_uniforms.find(kClipRegionFromModelSpaceMatrix);
    ASSERT_THAT(iter, Not(Eq(target_uniforms.end())));
    const mathfu::mat4 region_from_model(&iter->second[0]);
    const Sqt sqt = CalculateSqtFromMatrix(region_from_model);
    const mathfu::quat quat = mathfu::quat::FromEulerAngles(
        mathfu::vec3(90.f, 0.f, 0.f) * kDegreesToRadians);
    EXPECT_THAT(sqt.translation,
                NearMathfuVec3(mathfu::vec3(9.f, 8.f, 7.f), kEpsilon));
    EXPECT_THAT(sqt.rotation, NearMathfuQuat(quat, kEpsilon));
    EXPECT_THAT(sqt.scale,
                NearMathfuVec3(mathfu::vec3(10.f, 20.f, 30.f), kEpsilon));
  }

  entity_factory->Destroy(parent);
  set_uniforms_.clear();
  shader_clip_system->Update();

  // After destroy there should be no more uniforms set.
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(parent), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(target), set_uniforms_.end());
}

// Tests that creating an orphaned leaf TargetDef first, then adding its parent
// to a region will connect the target to the region.
TEST_F(ClipSystemTest, OrphanedLeafTargetDef) {
  auto* entity_factory = registry_.Get<EntityFactory>();
  auto* shader_clip_system = registry_.Get<ShaderClipSystem>();
  auto* transform_system = registry_.Get<TransformSystem>();

  BlueprintTree clip_blueprint;
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(1.f, 2.f, 3.f);
    transform.rotation = mathfu::vec3(90.f, 0.f, 0.f);
    transform.scale = mathfu::vec3(1.f, 2.f, 3.f);
    ShaderClipDefT clip;
    clip.min_in_clip_region_space = mathfu::vec3(-4.f, -5.f, -6.f);
    clip.max_in_clip_region_space = mathfu::vec3(7.f, 8.f, 9.f);
    clip_blueprint.Write(&transform);
    clip_blueprint.Write(&clip);
  }
  BlueprintTree parent_blueprint;
  BlueprintTree* target_blueprint = parent_blueprint.NewChild();
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(-2.f, -4.f, -6.f);
    parent_blueprint.Write(&transform);
  }
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(11.f, 12.f, 13.f);
    transform.rotation = mathfu::vec3(90.f, 0.f, 0.f);
    transform.scale = mathfu::vec3(10.f, 20.f, 30.f);
    ShaderClipTargetDefT target;
    target_blueprint->Write(&transform);
    target_blueprint->Write(&target);
  }

  const Entity region = entity_factory->Create(&clip_blueprint);
  // The target is attached to the parent, and the parent starts off isolated.
  const Entity parent = entity_factory->Create(&parent_blueprint);
  const std::vector<Entity>* children = transform_system->GetChildren(parent);
  ASSERT_EQ(false, children->empty());
  const Entity& target = children->at(0);

  // The region and parent should not get uniforms, whereas the target does.
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(parent), set_uniforms_.end());
  auto iter = set_uniforms_.find(target);
  ASSERT_THAT(iter, Not(Eq(set_uniforms_.end())));
  auto& inactive_target_uniforms = iter->second;
  // The inactive target should have its uniforms set to passive values.
  {
    auto iter = inactive_target_uniforms.find(
        kClipRegionFromModelSpaceMatrix);
    const mathfu::mat4 region_from_model(&iter->second[0]);
    iter = inactive_target_uniforms.find(kMinInClipRegionSpace);
    const mathfu::vec3 min(&iter->second[0]);
    iter = inactive_target_uniforms.find(kMaxInClipRegionSpace);
    const mathfu::vec3 max(&iter->second[0]);
    EXPECT_THAT(region_from_model, NearMathfu(mathfu::mat4(0.f), kEpsilon));
    EXPECT_THAT(min, NearMathfuVec3(-mathfu::kOnes3f, kEpsilon));
    EXPECT_THAT(max, NearMathfuVec3(mathfu::kOnes3f, kEpsilon));
  }

  // No uniforms should get updated.
  set_uniforms_.clear();
  shader_clip_system->Update();
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(parent), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(target), set_uniforms_.end());

  // Add the parent to the region and the target should now get uniforms.
  transform_system->AddChild(region, parent);
  shader_clip_system->Update();
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(parent), set_uniforms_.end());
  iter = set_uniforms_.find(target);
  ASSERT_THAT(iter, Not(Eq(set_uniforms_.end())));
  auto& target_uniforms = iter->second;
  // The min and max uniforms should match the region's ClipDef.
  {
    auto iter = target_uniforms.find(kMinInClipRegionSpace);
    ASSERT_THAT(iter, Not(Eq(target_uniforms.end())));
    const mathfu::vec3 min(&iter->second[0]);
    EXPECT_THAT(min, NearMathfuVec3(mathfu::vec3(-4.f, -5.f, -6.f), kEpsilon));
  }
  {
    auto iter = target_uniforms.find(kMaxInClipRegionSpace);
    ASSERT_THAT(iter, Not(Eq(target_uniforms.end())));
    const mathfu::vec3 max(&iter->second[0]);
    EXPECT_THAT(max, NearMathfuVec3(mathfu::vec3(7.f, 8.f, 9.f), kEpsilon));
  }
  // The parent slightly modifies the child's location.
  {
    auto iter = target_uniforms.find(kClipRegionFromModelSpaceMatrix);
    ASSERT_THAT(iter, Not(Eq(target_uniforms.end())));
    const mathfu::mat4 region_from_model(&iter->second[0]);
    const Sqt sqt = CalculateSqtFromMatrix(region_from_model);
    const mathfu::quat quat = mathfu::quat::FromEulerAngles(
        mathfu::vec3(90.f, 0.f, 0.f) * kDegreesToRadians);
    EXPECT_THAT(sqt.translation,
                NearMathfuVec3(mathfu::vec3(9.f, 8.f, 7.f), kEpsilon));
    EXPECT_THAT(sqt.rotation, NearMathfuQuat(quat, kEpsilon));
    EXPECT_THAT(sqt.scale,
                NearMathfuVec3(mathfu::vec3(10.f, 20.f, 30.f), kEpsilon));
  }

  entity_factory->Destroy(parent);
  set_uniforms_.clear();
  shader_clip_system->Update();

  // After destroy there should be no more uniforms set.
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(parent), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(target), set_uniforms_.end());
}

// Tests a manually enabled clip target's ownership of its descendents as the
// hierarchy is manipulated.
TEST_F(ClipSystemTest, Ownership) {
  auto* entity_factory = registry_.Get<EntityFactory>();
  auto* shader_clip_system = registry_.Get<ShaderClipSystem>();
  auto* transform_system = registry_.Get<TransformSystem>();

  Blueprint clip_blueprint;
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(1.f, 2.f, 3.f);
    ShaderClipDefT clip;
    clip.min_in_clip_region_space = mathfu::vec3(-4.f, -5.f, -6.f);
    clip.max_in_clip_region_space = mathfu::vec3(7.f, 8.f, 9.f);
    clip_blueprint.Write(&transform);
    clip_blueprint.Write(&clip);
  }
  Blueprint empty_blueprint;
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(1.f, 2.f, 3.f);
    empty_blueprint.Write(&transform);
  }

  const Entity region = entity_factory->Create(&clip_blueprint);
  const Entity parent = entity_factory->Create(&empty_blueprint);
  const Entity target_child = entity_factory->Create(&empty_blueprint);
  const Entity target_grandchild = entity_factory->Create(&empty_blueprint);

  // No targets enabled, so no uniforms set.
  shader_clip_system->Update();
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(parent), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(target_child), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(target_grandchild), set_uniforms_.end());

  // Test adding targets to a region.
  transform_system->AddChild(parent, target_child);
  transform_system->AddChild(region, parent);
  shader_clip_system->AddTarget(region, target_child);
  shader_clip_system->Update();
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(parent), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(target_grandchild), set_uniforms_.end());

  auto iter = set_uniforms_.find(target_child);
  ASSERT_THAT(iter, Not(Eq(set_uniforms_.end())));
  auto& target_child_uniforms = iter->second;
  {
    auto iter = target_child_uniforms.find(kClipRegionFromModelSpaceMatrix);
    ASSERT_THAT(iter, Not(Eq(target_child_uniforms.end())));
    const mathfu::mat4 region_from_model(&iter->second[0]);
    const Sqt sqt = CalculateSqtFromMatrix(region_from_model);
    EXPECT_THAT(sqt.translation,
                NearMathfuVec3(mathfu::vec3(2.f, 4.f, 6.f), kEpsilon));
  }

  // Next, test adding a child to an existing clip target.
  transform_system->AddChild(target_child, target_grandchild);
  shader_clip_system->Update();
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(parent), set_uniforms_.end());

  iter = set_uniforms_.find(target_grandchild);
  ASSERT_THAT(iter, Not(Eq(set_uniforms_.end())));
  auto& target_grandchild_uniforms = iter->second;
  {
    auto iter = target_grandchild_uniforms.find(
        kClipRegionFromModelSpaceMatrix);
    ASSERT_THAT(iter, Not(Eq(target_grandchild_uniforms.end())));
    const mathfu::mat4 region_from_model(&iter->second[0]);
    const Sqt sqt = CalculateSqtFromMatrix(region_from_model);
    EXPECT_THAT(sqt.translation,
                NearMathfuVec3(mathfu::vec3(3.f, 6.f, 9.f), kEpsilon));
  }

  // Lastly, test removing children from a manually enabled target.
  transform_system->RemoveParent(target_grandchild);
  {
    // The grandchild should have its uniforms set to passive values.
    auto iter = target_grandchild_uniforms.find(
        kClipRegionFromModelSpaceMatrix);
    const mathfu::mat4 region_from_model(&iter->second[0]);
    iter = target_grandchild_uniforms.find(kMinInClipRegionSpace);
    const mathfu::vec3 min(&iter->second[0]);
    iter = target_grandchild_uniforms.find(kMaxInClipRegionSpace);
    const mathfu::vec3 max(&iter->second[0]);
    EXPECT_THAT(region_from_model, NearMathfu(mathfu::mat4(0.f), kEpsilon));
    EXPECT_THAT(min, NearMathfuVec3(-mathfu::kOnes3f, kEpsilon));
    EXPECT_THAT(max, NearMathfuVec3(mathfu::kOnes3f, kEpsilon));
  }

  // Future updates should not modify the removed grandchild.
  set_uniforms_.clear();
  shader_clip_system->Update();
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(parent), set_uniforms_.end());
  // Matrices are cached and if no changes are detected, no uniforms are set.
  EXPECT_EQ(set_uniforms_.find(target_child), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(target_grandchild), set_uniforms_.end());

  // Note that after being manually enabled, Targets still are clipped even if
  // the ancestry is changed.
  const Entity no_region = entity_factory->Create(&empty_blueprint);
  transform_system->AddChild(no_region, target_child);
  set_uniforms_.clear();
  shader_clip_system->Update();
  EXPECT_EQ(set_uniforms_.find(region), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(parent), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(target_grandchild), set_uniforms_.end());
  EXPECT_EQ(set_uniforms_.find(no_region), set_uniforms_.end());

  iter = set_uniforms_.find(target_child);
  ASSERT_THAT(iter, Not(Eq(set_uniforms_.end())));
  auto& moved_target_child_uniforms = iter->second;
  {
    // target_child is now only 1 generation from world space.
    auto iter =
        moved_target_child_uniforms.find(kClipRegionFromModelSpaceMatrix);
    ASSERT_THAT(iter, Not(Eq(moved_target_child_uniforms.end())));
    const mathfu::mat4 region_from_model(&iter->second[0]);
    const Sqt sqt = CalculateSqtFromMatrix(region_from_model);
    EXPECT_THAT(sqt.translation,
                NearMathfuVec3(mathfu::vec3(1.f, 2.f, 3.f), kEpsilon));
  }
}

}  // namespace
}  // namespace lull
