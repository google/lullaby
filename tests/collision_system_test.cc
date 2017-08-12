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

#include "lullaby/systems/collision/collision_system.h"
#include "gtest/gtest.h"
#include "lullaby/generated/collision_def_generated.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/blueprint.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/generated/transform_def_generated.h"

namespace lull {
namespace {

class CollisionSystemTest : public testing::Test {
 public:
  void SetUp() override {
    registry_.reset(new Registry());
    registry_->Create<Dispatcher>();

    auto* entity_factory = registry_->Create<EntityFactory>(registry_.get());
    entity_factory->CreateSystem<CollisionSystem>();
    entity_factory->CreateSystem<TransformSystem>();
    entity_factory->Initialize();
  }

 protected:
  std::unique_ptr<Registry> registry_;
};

TEST_F(CollisionSystemTest, CreateEnableDisableDestroy) {
  TransformDefT transform;
  CollisionDefT collision;

  Blueprint blueprint;
  blueprint.Write(&transform);
  blueprint.Write(&collision);

  auto* entity_factory = registry_->Get<EntityFactory>();
  const Entity entity = entity_factory->Create(&blueprint);
  EXPECT_NE(entity, kNullEntity);

  auto* collision_system = registry_->Get<CollisionSystem>();
  EXPECT_TRUE(collision_system->IsCollisionEnabled(entity));
  EXPECT_TRUE(collision_system->IsInteractionEnabled(entity));

  collision_system->DisableCollision(entity);
  EXPECT_FALSE(collision_system->IsCollisionEnabled(entity));
  collision_system->EnableCollision(entity);
  EXPECT_TRUE(collision_system->IsCollisionEnabled(entity));

  collision_system->DisableInteraction(entity);
  EXPECT_FALSE(collision_system->IsInteractionEnabled(entity));
  collision_system->EnableInteraction(entity);
  EXPECT_TRUE(collision_system->IsInteractionEnabled(entity));

  entity_factory->Destroy(entity);
  EXPECT_FALSE(collision_system->IsCollisionEnabled(entity));
  EXPECT_FALSE(collision_system->IsInteractionEnabled(entity));
}

TEST_F(CollisionSystemTest, CheckForCollision) {
  Blueprint blueprint1;
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(0.f, 0.f, -4.f);
    CollisionDefT collision;
    blueprint1.Write(&transform);
    blueprint1.Write(&collision);
  }

  auto* entity_factory = registry_->Get<EntityFactory>();
  const Entity entity1 = entity_factory->Create(&blueprint1);
  EXPECT_NE(entity1, kNullEntity);

  auto* collision_system = registry_->Get<CollisionSystem>();
  EXPECT_TRUE(collision_system->IsCollisionEnabled(entity1));

  auto* transform_system = registry_->Get<TransformSystem>();
  transform_system->SetAabb(entity1, Aabb(-mathfu::kOnes3f, mathfu::kOnes3f));

  static const float kEpsilon = 0.001f;

  // Shoot a ray that will hit the single entity.
  {
    const auto result = collision_system->CheckForCollision(
        Ray(mathfu::kZeros3f, -mathfu::kAxisZ3f));
    EXPECT_EQ(result.entity, entity1);
    EXPECT_NEAR(result.distance, 3.f, kEpsilon);
  }

  Blueprint blueprint2;
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(0.f, 0.f, -2.f);
    CollisionDefT collision;
    blueprint2.Write(&transform);
    blueprint2.Write(&collision);
  }

  const Entity entity2 = entity_factory->Create(&blueprint2);
  EXPECT_NE(entity2, kNullEntity);
  EXPECT_TRUE(collision_system->IsCollisionEnabled(entity2));

  transform_system->SetAabb(entity2, Aabb(-mathfu::kOnes3f / 2.f,
                                          mathfu::kOnes3f / 2.f));

  // Shoot a ray that will hit both entities, returning the closer entity.
  {
    const auto result = collision_system->CheckForCollision(
        Ray(mathfu::kZeros3f, -mathfu::kAxisZ3f));
    EXPECT_EQ(result.entity, entity2);
    EXPECT_NEAR(result.distance, 1.5f, kEpsilon);
  }

  // Shoot a ray that will miss the new closer entity and hit the back entity.
  {
    const auto result = collision_system->CheckForCollision(
        Ray(mathfu::vec3(0.75f, 0.f, 0.f), -mathfu::kAxisZ3f));
    EXPECT_EQ(result.entity, entity1);
    EXPECT_NEAR(result.distance, 3.f, kEpsilon);
  }

  // Shoot a ray that will miss both entities.
  {
    const auto result = collision_system->CheckForCollision(
        Ray(mathfu::vec3(2.f, 0.f, 0.f), -mathfu::kAxisZ3f));
    EXPECT_EQ(result.entity, kNullEntity);
    EXPECT_EQ(result.distance, kNoHitDistance);
  }
}

TEST_F(CollisionSystemTest, CheckForClip) {
  auto* entity_factory = registry_->Get<EntityFactory>();
  auto* collision_system = registry_->Get<CollisionSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();

  Blueprint parent_blueprint;
  Blueprint child_blueprint;
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(4.f, 4.f, -4.f);
    CollisionClipBoundsDefT clip_bounds;
    clip_bounds.aabb = Aabb(mathfu::vec3(0.4f), mathfu::vec3(0.6f));
    parent_blueprint.Write(&transform);
    parent_blueprint.Write(&clip_bounds);
  }
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(0.f, 0.f, 0.5f);
    transform.aabb =
        Aabb(mathfu::vec3(-1.f, -1.f, 0.f), mathfu::vec3(1.f, 1.f, 0.f));
    CollisionDefT collision;
    collision.clip_outside_bounds = true;
    child_blueprint.Write(&transform);
    child_blueprint.Write(&collision);
  }
  const Entity parent = entity_factory->Create(&parent_blueprint);
  const Entity child = entity_factory->Create(&child_blueprint);
  transform_system->AddChild(parent, child);

  EXPECT_NE(parent, kNullEntity);
  EXPECT_NE(child, kNullEntity);
  EXPECT_TRUE(collision_system->IsCollisionEnabled(child));

  static const float kEpsilon = 0.001f;

  // Shoot a ray that will hit the child inside the bounds.
  {
    const auto result = collision_system->CheckForCollision(
        Ray(mathfu::vec3(4.5f, 4.5f, 0.f), -mathfu::kAxisZ3f));
    EXPECT_EQ(result.entity, child);
    EXPECT_NEAR(result.distance, 3.5f, kEpsilon);
  }

  // Shoot a ray that will hit the child outside the bounds, so therefore not
  // register a collision.
  {
    const auto result = collision_system->CheckForCollision(
        Ray(mathfu::vec3(4.75f, 4.75f, 0.f), -mathfu::kAxisZ3f));
    EXPECT_EQ(result.entity, kNullEntity);
    EXPECT_EQ(result.distance, kNoHitDistance);
  }

  // Destroy the child's ClipContent component, then shoot the same ray and
  // expect a collision.
  collision_system->DisableClipping(child);
  {
    const auto result = collision_system->CheckForCollision(
        Ray(mathfu::vec3(4.75f, 4.75f, 0.f), -mathfu::kAxisZ3f));
    EXPECT_EQ(result.entity, child);
    EXPECT_NEAR(result.distance, 3.5f, kEpsilon);
  }
}

TEST_F(CollisionSystemTest, CheckForPointCollisions) {
  Blueprint blueprint1;
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(0.f, 0.f, -4.f);
    CollisionDefT collision;
    blueprint1.Write(&transform);
    blueprint1.Write(&collision);
  }

  auto* entity_factory = registry_->Get<EntityFactory>();
  const Entity entity1 = entity_factory->Create(&blueprint1);
  EXPECT_NE(entity1, kNullEntity);

  auto* collision_system = registry_->Get<CollisionSystem>();
  EXPECT_TRUE(collision_system->IsCollisionEnabled(entity1));

  auto* transform_system = registry_->Get<TransformSystem>();
  transform_system->SetAabb(entity1, Aabb(-mathfu::kOnes3f, mathfu::kOnes3f));

  // Check a point inside the single entity.
  {
    const auto result = collision_system->CheckForPointCollisions(
        mathfu::vec3(0.f, 0.f, -4.f));
    EXPECT_EQ(result.size(), static_cast<size_t>(1));
    EXPECT_EQ(result[0], entity1);
  }

  Blueprint blueprint2;
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(0.f, 0.f, -3.f);
    CollisionDefT collision;
    blueprint2.Write(&transform);
    blueprint2.Write(&collision);
  }

  const Entity entity2 = entity_factory->Create(&blueprint2);
  EXPECT_NE(entity2, kNullEntity);
  EXPECT_TRUE(collision_system->IsCollisionEnabled(entity2));

  transform_system->SetAabb(entity2, Aabb(-mathfu::kOnes3f, mathfu::kOnes3f));

  // Check a point inside both of the entities.
  {
    const auto result = collision_system->CheckForPointCollisions(
        mathfu::vec3(0.f, 0.f, -3.5f));
    EXPECT_EQ(result.size(), static_cast<size_t>(2));
    EXPECT_EQ(result[0], entity1);
    EXPECT_EQ(result[1], entity2);
  }

  // Check a point inside the new entity but not the old one
  {
    const auto result = collision_system->CheckForPointCollisions(
        mathfu::vec3(0.f, 0.f, -2.5f));
    EXPECT_EQ(result.size(), static_cast<size_t>(1));
    EXPECT_EQ(result[0], entity2);
  }

  // Check a point inside neither of the entities.
  {
    const auto result =
        collision_system->CheckForPointCollisions(mathfu::kZeros3f);
    EXPECT_EQ(result.size(), static_cast<size_t>(0));
  }
}

TEST_F(CollisionSystemTest, DefaultInteraction) {
  TransformDefT transform;
  CollisionDefT collision;

  Blueprint blueprint;
  blueprint.Write(&transform);
  blueprint.Write(&collision);

  auto* entity_factory = registry_->Get<EntityFactory>();
  const Entity entity = entity_factory->Create(&blueprint);
  EXPECT_NE(entity, kNullEntity);

  auto* collision_system = registry_->Get<CollisionSystem>();
  EXPECT_TRUE(collision_system->IsCollisionEnabled(entity));
  EXPECT_TRUE(collision_system->IsInteractionEnabled(entity));

  collision_system->DisableInteraction(entity);
  EXPECT_FALSE(collision_system->IsInteractionEnabled(entity));

  collision_system->RestoreInteraction(entity);
  EXPECT_TRUE(collision_system->IsInteractionEnabled(entity));

  collision_system->DisableDefaultInteraction(entity);
  EXPECT_TRUE(collision_system->IsInteractionEnabled(entity));

  collision_system->RestoreInteraction(entity);
  EXPECT_FALSE(collision_system->IsInteractionEnabled(entity));
}

TEST_F(CollisionSystemTest, DefaultInteractionDescendants) {
  TransformDefT transform;
  CollisionDefT collision;

  Blueprint blueprint;
  blueprint.Write(&transform);
  blueprint.Write(&collision);

  auto* entity_factory = registry_->Get<EntityFactory>();
  const Entity parent = entity_factory->Create(&blueprint);
  EXPECT_NE(parent, kNullEntity);

  auto* collision_system = registry_->Get<CollisionSystem>();
  EXPECT_TRUE(collision_system->IsCollisionEnabled(parent));
  EXPECT_TRUE(collision_system->IsInteractionEnabled(parent));

  const Entity child = entity_factory->Create(&blueprint);
  EXPECT_NE(child, kNullEntity);

  EXPECT_TRUE(collision_system->IsCollisionEnabled(parent));
  EXPECT_TRUE(collision_system->IsInteractionEnabled(parent));

  registry_->Get<TransformSystem>()->AddChild(parent, child);

  collision_system->DisableInteractionDescendants(parent);

  EXPECT_FALSE(collision_system->IsInteractionEnabled(parent));
  EXPECT_FALSE(collision_system->IsInteractionEnabled(child));

  collision_system->RestoreInteractionDescendants(parent);

  EXPECT_TRUE(collision_system->IsInteractionEnabled(parent));
  EXPECT_TRUE(collision_system->IsInteractionEnabled(child));
}

}  // namespace
}  // namespace lull
