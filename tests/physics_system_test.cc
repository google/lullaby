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

#include "lullaby/systems/physics/physics_system.h"
#include "gtest/gtest.h"
#include "lullaby/events/physics_events.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/generated/rigid_body_def_generated.h"
#include "tests/mathfu_matchers.h"
#include "tests/portable_test_macros.h"
#include "lullaby/generated/transform_def_generated.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::Ne;
using testing::NearMathfuVec3;
using testing::NearMathfuQuat;

const float kFrameSeconds = 1.f / 60.f;
const Clock::duration kFrameDuration = DurationFromSeconds(kFrameSeconds);

class PhysicsSystemTest : public ::testing::Test {
 public:
  void SetUp() override {
    registry_ = MakeUnique<Registry>();
    registry_->Create<Dispatcher>();

    auto* entity_factory = registry_->Create<EntityFactory>(registry_.get());
    entity_factory->CreateSystem<DispatcherSystem>();
    entity_factory->CreateSystem<TransformSystem>();
    entity_factory->CreateSystem<PhysicsSystem>();
    entity_factory->Initialize();
  }

  void TearDown() override {
  }

  // Creates a 2x2x2 rigid body at the given position with the given types.
  Entity CreateBasicRigidBody(
      const mathfu::vec3& position = mathfu::kZeros3f,
      RigidBodyType type = RigidBodyType::RigidBodyType_Dynamic,
      ColliderType collider = ColliderType::ColliderType_Standard) {
    auto* entity_factory = registry_->Get<EntityFactory>();

    Blueprint blueprint(512);

    TransformDefT transform;
    transform.aabb.min = -mathfu::kOnes3f;
    transform.aabb.max = mathfu::kOnes3f;
    transform.position = position;
    blueprint.Write(&transform);

    RigidBodyDefT rigid_body;
    rigid_body.type = type;
    rigid_body.collider_type = collider;
    blueprint.Write(&rigid_body);

    return entity_factory->Create(&blueprint);
  }

 protected:
  std::unique_ptr<Registry> registry_;
};

// Test enabling and disabling physics.
TEST_F(PhysicsSystemTest, EnableAndDisablePhysics) {
  auto* entity_factory = registry_->Get<EntityFactory>();
  auto* physics_system = registry_->Get<PhysicsSystem>();

  // Test that physics is enabled by default.
  const Entity entity = CreateBasicRigidBody();
  EXPECT_THAT(entity, Ne(kNullEntity));
  EXPECT_TRUE(physics_system->IsPhysicsEnabled(entity));

  // Enable and disable physics.
  physics_system->DisablePhysics(entity);
  EXPECT_FALSE(physics_system->IsPhysicsEnabled(entity));

  physics_system->EnablePhysics(entity);
  EXPECT_TRUE(physics_system->IsPhysicsEnabled(entity));

  // Test that the enable_on_create field is respected.
  Blueprint disabled_blueprint(512);
  {
    TransformDefT transform;
    disabled_blueprint.Write(&transform);

    RigidBodyDefT rigid_body;
    rigid_body.enable_on_create = false;
    disabled_blueprint.Write(&rigid_body);
  }

  const Entity disabled = entity_factory->Create(&disabled_blueprint);
  EXPECT_THAT(disabled, Ne(kNullEntity));
  EXPECT_FALSE(physics_system->IsPhysicsEnabled(disabled));

  // Test that the an Entity with no RigidBodyDef reports physics disabled.
  Blueprint no_physics_blueprint(512);
  {
    TransformDefT transform;
    no_physics_blueprint.Write(&transform);
  }

  const Entity no_physics = entity_factory->Create(&no_physics_blueprint);
  EXPECT_THAT(no_physics, Ne(kNullEntity));
  EXPECT_FALSE(physics_system->IsPhysicsEnabled(no_physics));
  physics_system->EnablePhysics(no_physics);
  EXPECT_FALSE(physics_system->IsPhysicsEnabled(no_physics));
}

// Test contact events. Use a Static and Kinematic trigger for simplicity.
TEST_F(PhysicsSystemTest, ContactEvents) {
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  auto* physics_system = registry_->Get<PhysicsSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();

  // Create a Static Trigger box at the origin.
  const Entity origin = CreateBasicRigidBody(
      mathfu::kZeros3f, RigidBodyType::RigidBodyType_Static,
      ColliderType::ColliderType_Trigger);
  EXPECT_THAT(origin, Ne(kNullEntity));

  int origin_contacts = 0;
  dispatcher_system->Connect(
      origin, this, [&origin_contacts](const EnterPhysicsContactEvent& e) {
        ++origin_contacts;
      });
  dispatcher_system->Connect(
      origin, this, [&origin_contacts](const ExitPhysicsContactEvent& e) {
        --origin_contacts;
      });

  // Create a 2x2x2 Kinematic Trigger box above the origin and out of contact
  // with the first Entity.
  const Entity kinematic = CreateBasicRigidBody(
      mathfu::vec3(0.f, 3.f, 0.f), RigidBodyType::RigidBodyType_Kinematic,
      ColliderType::ColliderType_Trigger);
  EXPECT_THAT(kinematic, Ne(kNullEntity));

  int kinematic_contacts = 0;
  dispatcher_system->Connect(
      kinematic, this, [&kinematic_contacts]
      (const EnterPhysicsContactEvent& e) {
        ++kinematic_contacts;
      });
  dispatcher_system->Connect(
      kinematic, this, [&kinematic_contacts]
      (const ExitPhysicsContactEvent& e) {
        --kinematic_contacts;
      });

  // Advance the PhysicsSystem and confirm they are not in contact. Ensure order
  // doesn't matter.
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_FALSE(physics_system->AreInContact(origin, kinematic));
  EXPECT_FALSE(physics_system->AreInContact(kinematic, origin));

  EXPECT_THAT(origin_contacts, Eq(0));
  EXPECT_THAT(kinematic_contacts, Eq(0));

  // Move the kinematic object down, then re-do contact tests.
  transform_system->SetLocalTranslation(
      kinematic, mathfu::vec3(0.f, 1.5f, 0.f));

  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_TRUE(physics_system->AreInContact(origin, kinematic));
  EXPECT_TRUE(physics_system->AreInContact(kinematic, origin));

  EXPECT_THAT(origin_contacts, Eq(1));
  EXPECT_THAT(kinematic_contacts, Eq(1));

  // Advancing the PhysicsSystem again changes nothing.
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_TRUE(physics_system->AreInContact(origin, kinematic));
  EXPECT_TRUE(physics_system->AreInContact(kinematic, origin));

  EXPECT_THAT(origin_contacts, Eq(1));
  EXPECT_THAT(kinematic_contacts, Eq(1));

  // Move the kinematic object back up, then re-do contact tests.
  transform_system->SetLocalTranslation(kinematic, mathfu::vec3(0.f, 3.f, 0.f));

  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_FALSE(physics_system->AreInContact(origin, kinematic));
  EXPECT_FALSE(physics_system->AreInContact(kinematic, origin));

  EXPECT_THAT(origin_contacts, Eq(0));
  EXPECT_THAT(kinematic_contacts, Eq(0));
}

// Test that rigid bodies are added and removed to the world at the right time.
// Use contact to determine if both bodies are in the world.
TEST_F(PhysicsSystemTest, RigidBodyInSimulation) {
  auto* physics_system = registry_->Get<PhysicsSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();

  // Create a Static Trigger box at the origin.
  const Entity origin = CreateBasicRigidBody(
      mathfu::kZeros3f, RigidBodyType::RigidBodyType_Static,
      ColliderType::ColliderType_Trigger);
  EXPECT_THAT(origin, Ne(kNullEntity));

  // Create a 2x2x2 Kinematic Trigger box also at the origin, meanign the two
  // will always be in contact if both are in the simulation.
  const Entity kinematic = CreateBasicRigidBody(
      mathfu::kZeros3f, RigidBodyType::RigidBodyType_Kinematic,
      ColliderType::ColliderType_Trigger);
  EXPECT_THAT(kinematic, Ne(kNullEntity));

  // Advance the PhysicsSystem and confirm they in contact.
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_TRUE(physics_system->AreInContact(origin, kinematic));

  // Disabling physics on one of the Entities results in no contact.
  physics_system->DisablePhysics(origin);
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_FALSE(physics_system->AreInContact(origin, kinematic));

  // Re-enabling it will trigger contact events.
  physics_system->EnablePhysics(origin);
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_TRUE(physics_system->AreInContact(origin, kinematic));

  // Likewise, disabling one of Entities entirely will result in no contact.
  transform_system->Disable(origin);
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_FALSE(physics_system->AreInContact(origin, kinematic));

  // Re-enabling it will trigger contact events.
  transform_system->Enable(origin);
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_TRUE(physics_system->AreInContact(origin, kinematic));

  // Disable physics and disable the Entity itself, then confirm no contact.
  physics_system->DisablePhysics(origin);
  transform_system->Disable(origin);
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_FALSE(physics_system->AreInContact(origin, kinematic));

  // Just enabling physics doesn't result in contact.
  physics_system->EnablePhysics(origin);
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_FALSE(physics_system->AreInContact(origin, kinematic));
  physics_system->DisablePhysics(origin);

  // Just enabling the Entity doesn't result in contact.
  transform_system->Enable(origin);
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_FALSE(physics_system->AreInContact(origin, kinematic));
  transform_system->Disable(origin);

  // Enabling physics and the Entity results in contact.
  physics_system->EnablePhysics(origin);
  transform_system->Enable(origin);
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_TRUE(physics_system->AreInContact(origin, kinematic));
}

// Test that scale changes take effect.
TEST_F(PhysicsSystemTest, ScaleChange) {
  auto* physics_system = registry_->Get<PhysicsSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();

  // Create a Static Trigger box at the origin.
  const Entity origin = CreateBasicRigidBody(
      mathfu::kZeros3f, RigidBodyType::RigidBodyType_Static,
      ColliderType::ColliderType_Trigger);
  EXPECT_THAT(origin, Ne(kNullEntity));

  // Create a 2x2x2 Kinematic Trigger box above the origin and out of contact
  // with the first Entity.
  const Entity kinematic = CreateBasicRigidBody(
      mathfu::vec3(0.f, 3.f, 0.f), RigidBodyType::RigidBodyType_Kinematic,
      ColliderType::ColliderType_Trigger);
  EXPECT_THAT(kinematic, Ne(kNullEntity));

  // Advance the PhysicsSystem and confirm they are not in contact.
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_FALSE(physics_system->AreInContact(origin, kinematic));

  // Scale the kinematic object into contact and advance the PhysicsSystem.
  transform_system->SetLocalScale(kinematic, 4.f * mathfu::kOnes3f);
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_TRUE(physics_system->AreInContact(origin, kinematic));

  // Scale the kinematic object out of contact and advance the PhysicsSystem.
  transform_system->SetLocalScale(kinematic, mathfu::kOnes3f);
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_FALSE(physics_system->AreInContact(origin, kinematic));
}

// Test that AABB changes take effect.
TEST_F(PhysicsSystemTest, AabbChange) {
  auto* physics_system = registry_->Get<PhysicsSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();

  // Create a Static Trigger box at the origin.
  const Entity origin = CreateBasicRigidBody(
      mathfu::kZeros3f, RigidBodyType::RigidBodyType_Static,
      ColliderType::ColliderType_Trigger);
  EXPECT_THAT(origin, Ne(kNullEntity));

  // Create a 2x2x2 Kinematic Trigger box above the origin and out of contact
  // with the first Entity.
  const Entity kinematic = CreateBasicRigidBody(
      mathfu::vec3(0.f, 3.f, 0.f), RigidBodyType::RigidBodyType_Kinematic,
      ColliderType::ColliderType_Trigger);
  EXPECT_THAT(kinematic, Ne(kNullEntity));

  // Advance the PhysicsSystem and confirm they are not in contact.
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_FALSE(physics_system->AreInContact(origin, kinematic));

  // Set the kinematic object's AABB into contact and advance the PhysicsSystem.
  transform_system->SetAabb(
      kinematic, Aabb(-4.f * mathfu::kOnes3f, 4.f * mathfu::kOnes3f));
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_TRUE(physics_system->AreInContact(origin, kinematic));

  // Change the kinematic object's AABB to be the same scale, but entirely in
  // the positive axis so the objects exit contact.
  transform_system->SetAabb(kinematic,
                            Aabb(mathfu::kZeros3f, 8.f * mathfu::kOnes3f));
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_FALSE(physics_system->AreInContact(origin, kinematic));
}

// Test that Static objects don't move even if their transform is updated.
TEST_F(PhysicsSystemTest, StaticBody) {
  auto* physics_system = registry_->Get<PhysicsSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();

  // Create a Static Trigger box at the origin.
  const Entity origin = CreateBasicRigidBody(
      mathfu::kZeros3f, RigidBodyType::RigidBodyType_Static,
      ColliderType::ColliderType_Trigger);
  EXPECT_THAT(origin, Ne(kNullEntity));

  // Create a Static Trigger box above the origin and out of contact with the
  // other box.
  const Entity above = CreateBasicRigidBody(
      mathfu::vec3(0.f, 3.f, 0.f), RigidBodyType::RigidBodyType_Static,
      ColliderType::ColliderType_Trigger);
  EXPECT_THAT(above, Ne(kNullEntity));

  // Advance the PhysicsSystem and confirm they are not in contact.
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_FALSE(physics_system->AreInContact(origin, above));

  // Move the second object down into contact, then re-do contact tests and
  // ensure nothing changes because it is Static.
  transform_system->SetLocalTranslation(above, mathfu::vec3(0.f, 1.f, 0.f));
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_FALSE(physics_system->AreInContact(origin, above));

  // Scaling the box to an enourmous size won't make a difference either.
  transform_system->SetLocalScale(above, 100.f * mathfu::kOnes3f);
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_FALSE(physics_system->AreInContact(origin, above));
}

// Test that Dynamic bodies have their transform updated.
TEST_F(PhysicsSystemTest, DynamicBody) {
  auto* physics_system = registry_->Get<PhysicsSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();

  // Disable gravity.
  physics_system->SetGravity(mathfu::kZeros3f);

  // Create a Dynamic box at the origin.
  const Entity entity = CreateBasicRigidBody(
      mathfu::kZeros3f, RigidBodyType::RigidBodyType_Dynamic);
  EXPECT_THAT(entity, Ne(kNullEntity));

  // Advance the PhysicsSystem and confirm that there no changes in transform.
  const Sqt first_sqt = *transform_system->GetSqt(entity);
  physics_system->AdvanceFrame(kFrameDuration);
  const Sqt second_sqt = *transform_system->GetSqt(entity);

  EXPECT_THAT(second_sqt.translation,
              NearMathfuVec3(first_sqt.translation, kDefaultEpsilon));
  EXPECT_THAT(second_sqt.rotation,
              NearMathfuQuat(first_sqt.rotation, kDefaultEpsilon));

  // Set a linear velocity, then check that translation changes. Don't worry
  // about the exact difference.
  physics_system->SetLinearVelocity(entity, mathfu::kOnes3f);
  physics_system->AdvanceFrame(kFrameDuration);
  const Sqt third_sqt = *transform_system->GetSqt(entity);

  EXPECT_THAT(third_sqt.translation,
              Not(NearMathfuVec3(second_sqt.translation, kDefaultEpsilon)));
  EXPECT_THAT(third_sqt.rotation,
              NearMathfuQuat(second_sqt.rotation, kDefaultEpsilon));

  // Set an angular velocity, then check that rotation changes. Don't worry
  // about the exact difference.
  physics_system->SetAngularVelocity(entity, mathfu::kOnes3f);
  physics_system->AdvanceFrame(kFrameDuration);
  const Sqt fourth_sqt = *transform_system->GetSqt(entity);

  EXPECT_THAT(fourth_sqt.translation,
              Not(NearMathfuVec3(third_sqt.translation, kDefaultEpsilon)));
  EXPECT_THAT(fourth_sqt.rotation,
              Not(NearMathfuQuat(third_sqt.rotation, kDefaultEpsilon)));

  // Zero out the velocities and check that nothing changes.
  physics_system->SetLinearVelocity(entity, mathfu::kZeros3f);
  physics_system->SetAngularVelocity(entity, mathfu::kZeros3f);
  physics_system->AdvanceFrame(kFrameDuration);
  const Sqt fifth_sqt = *transform_system->GetSqt(entity);

  EXPECT_THAT(fifth_sqt.translation,
              NearMathfuVec3(fourth_sqt.translation, kDefaultEpsilon));
  EXPECT_THAT(fifth_sqt.rotation,
              NearMathfuQuat(fourth_sqt.rotation, kDefaultEpsilon));

  // Re-enable gravity and check that the position changes.
  physics_system->SetGravity(mathfu::vec3(0.f, -10.f, 0.f));
  physics_system->AdvanceFrame(kFrameDuration);
  const Sqt sixth_sqt = *transform_system->GetSqt(entity);

  EXPECT_THAT(sixth_sqt.translation,
              Not(NearMathfuVec3(fifth_sqt.translation, kDefaultEpsilon)));
  EXPECT_THAT(sixth_sqt.rotation,
              NearMathfuQuat(fifth_sqt.rotation, kDefaultEpsilon));
}

// Test the center of mass translation.
TEST_F(PhysicsSystemTest, CenterOfMassTranslation) {
  auto* physics_system = registry_->Get<PhysicsSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();
  auto* entity_factory = registry_->Get<EntityFactory>();

  // Create a Static Trigger box at the origin.
  const Entity origin = CreateBasicRigidBody(
      mathfu::kZeros3f, RigidBodyType::RigidBodyType_Static,
      ColliderType::ColliderType_Trigger);
  EXPECT_THAT(origin, Ne(kNullEntity));

  // Create a 2x2x2 Kinematic Trigger box also at the origin, but give it a
  // center of mass translation.
  Blueprint blueprint(512);
  {
    TransformDefT transform;
    transform.aabb.min = -mathfu::kOnes3f;
    transform.aabb.max = mathfu::kOnes3f;
    blueprint.Write(&transform);

    RigidBodyDefT rigid_body;
    rigid_body.type = RigidBodyType::RigidBodyType_Kinematic;
    rigid_body.collider_type = ColliderType::ColliderType_Trigger;
    rigid_body.center_of_mass_translation = mathfu::vec3(0.f, 3.f, 0.f);
    blueprint.Write(&rigid_body);
  }
  const Entity kinematic = entity_factory->Create(&blueprint);
  EXPECT_THAT(kinematic, Ne(kNullEntity));

  // Advance the PhysicsSystem and confirm they are not in contact.
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_FALSE(physics_system->AreInContact(origin, kinematic));

  // Move the kinematic object down to offset its center of mass translation and
  // verify that it is in contact due to said translation (even though based on
  // its position and AABB, it shouldn't be).
  transform_system->SetLocalTranslation(kinematic,
                                        mathfu::vec3(0.f, -3.f, 0.f));
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_TRUE(physics_system->AreInContact(origin, kinematic));

  // Move it back out of contact. Confirm that the Entity positions are indeed
  // the same.
  transform_system->SetLocalTranslation(kinematic, mathfu::kZeros3f);
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_FALSE(physics_system->AreInContact(origin, kinematic));
  EXPECT_THAT(transform_system->GetLocalTranslation(kinematic),
              Eq(transform_system->GetLocalTranslation(origin)));
}

// Test that Dynamic bodies have their transforms updated appropriately based
// on the length of the frame.
TEST_F(PhysicsSystemTest, MultipleTimesteps) {
  auto* physics_system = registry_->Get<PhysicsSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();

  // Disable gravity.
  physics_system->SetGravity(mathfu::kZeros3f);

  // Create a Dynamic box at the origin.
  const Entity entity = CreateBasicRigidBody(
      mathfu::kZeros3f, RigidBodyType::RigidBodyType_Dynamic);
  EXPECT_THAT(entity, Ne(kNullEntity));

  // Give the body a simple linear velocity.
  const mathfu::vec3 velocity = mathfu::kOnes3f;
  physics_system->SetLinearVelocity(entity, velocity);
  const Sqt* sqt = transform_system->GetSqt(entity);

  // Give the system a typical frame and ensure that the position changes by the
  // expected amount.
  mathfu::vec3 next_position = sqt->translation + kFrameSeconds * velocity;
  physics_system->AdvanceFrame(kFrameDuration);
  EXPECT_THAT(sqt->translation, NearMathfuVec3(next_position, kDefaultEpsilon));

  // Give the system a longer frame and ensure that the position changes by the
  // expected amount.
  next_position = sqt->translation + (3.f * kFrameSeconds) * velocity;
  physics_system->AdvanceFrame(3 * kFrameDuration);
  EXPECT_THAT(sqt->translation, NearMathfuVec3(next_position, kDefaultEpsilon));

  // Give the system a really long frame and it will fail to process it all at
  // once and time will be "lost".
  next_position = sqt->translation + (10.f * kFrameSeconds) * velocity;
  physics_system->AdvanceFrame(10 * kFrameDuration);
  EXPECT_THAT(sqt->translation,
              Not(NearMathfuVec3(next_position, kDefaultEpsilon)));

  // Give the system two really small frames and it will only update when a full
  // fixed timestep has been reached.
  const mathfu::vec3 old_position = sqt->translation;
  next_position = sqt->translation + (kFrameSeconds) * velocity;

  physics_system->AdvanceFrame(kFrameDuration / 2);
  EXPECT_THAT(sqt->translation, NearMathfuVec3(old_position, kDefaultEpsilon));

  physics_system->AdvanceFrame(kFrameDuration / 2);
  EXPECT_THAT(sqt->translation, NearMathfuVec3(next_position, kDefaultEpsilon));
}

}  // namespace
}  // namespace lull
