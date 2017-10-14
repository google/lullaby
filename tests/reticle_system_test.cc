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

#include "lullaby/systems/reticle/reticle_system.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/generated/collision_def_generated.h"
#include "lullaby/events/input_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/systems/collision/collision_system.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/render/testing/mock_render_system_impl.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/generated/reticle_behaviour_def_generated.h"
#include "lullaby/generated/reticle_def_generated.h"
#include "lullaby/generated/transform_def_generated.h"
#include "lullaby/util/clock.h"

namespace lull {
namespace {

const float kEpsilon = 0.001f;
const Clock::duration kDeltaTime = std::chrono::milliseconds(17);
const Clock::duration kLongPressTime = std::chrono::milliseconds(500);

using ::testing::_;

class ReticleSystemTest : public testing::Test {
 public:
  void SetUp() override {
    registry_.reset(new Registry());
    registry_->Register(std::unique_ptr<Dispatcher>(new Dispatcher()));
    registry_->Create<InputManager>();

    auto* entity_factory = registry_->Create<EntityFactory>(registry_.get());
    entity_factory->CreateSystem<CollisionSystem>();
    entity_factory->CreateSystem<DispatcherSystem>();

    auto* render_system = entity_factory->CreateSystem<RenderSystem>();
    mock_render_system_ = render_system->GetImpl();

    entity_factory->CreateSystem<ReticleSystem>();
    entity_factory->CreateSystem<TransformSystem>();

    EXPECT_CALL(*mock_render_system_, Initialize());
    entity_factory->Initialize();
  }

 protected:
  void Create3DofDevice() {
    InputManager::DeviceParams params;
    params.has_position_dof = false;
    params.has_rotation_dof = true;
    params.has_touchpad = false;
    params.has_touch_gesture = false;
    params.has_scroll = false;
    params.num_joysticks = 0;
    params.num_buttons = 1;
    params.num_eyes = 0;
    params.long_press_time = kLongPressTime;

    auto* input = registry_->Get<InputManager>();
    input->ConnectDevice(InputManager::kController, params);
    EXPECT_TRUE(input->IsConnected(InputManager::kController));
  }

  void ExpectReticleCreationUniforms() {
    EXPECT_CALL(*mock_render_system_, SetUniform(_, _, _, 4, 1));
    EXPECT_CALL(*mock_render_system_, SetUniform(_, _, _, 1, 1)).Times(6);
  }

  void ExpectReticleUpdateUniforms() {
    EXPECT_CALL(*mock_render_system_, SetUniform(_, _, _, 1, 1)).Times(1);
    EXPECT_CALL(*mock_render_system_, SetUniform(_, _, _, 4, 1)).Times(1);
  }

  std::unique_ptr<Registry> registry_;
  RenderSystemImpl* mock_render_system_;
};

TEST_F(ReticleSystemTest, CreateDestroy) {
  Blueprint blueprint;

  TransformDefT transform;
  blueprint.Write(&transform);
  ReticleDefT reticle;
  blueprint.Write(&reticle);

  ExpectReticleCreationUniforms();
  auto* entity_factory = registry_->Get<EntityFactory>();
  const Entity entity = entity_factory->Create(&blueprint);

  EXPECT_NE(entity, kNullEntity);

  auto* reticle_system = registry_->Get<ReticleSystem>();

  EXPECT_EQ(reticle_system->GetReticle(), entity);
  EXPECT_EQ(reticle_system->GetTarget(), kNullEntity);
  {
    const Ray collision_ray = reticle_system->GetCollisionRay();
    EXPECT_NEAR(collision_ray.origin.x, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.origin.y, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.origin.z, 0.f, kEpsilon);

    EXPECT_NEAR(collision_ray.direction.x, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.y, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.z, -1.f, kEpsilon);
  }
  EXPECT_EQ(reticle_system->GetActiveDevice(),
            InputManager::kMaxNumDeviceTypes);

  reticle_system->Destroy(entity);

  EXPECT_EQ(reticle_system->GetReticle(), kNullEntity);
  EXPECT_EQ(reticle_system->GetTarget(), kNullEntity);
  {
    const Ray collision_ray = reticle_system->GetCollisionRay();
    EXPECT_NEAR(collision_ray.origin.x, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.origin.y, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.origin.z, 0.f, kEpsilon);

    EXPECT_NEAR(collision_ray.direction.x, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.y, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.z, -1.f, kEpsilon);
  }
  EXPECT_EQ(reticle_system->GetActiveDevice(),
            InputManager::kMaxNumDeviceTypes);
}

TEST_F(ReticleSystemTest, NoInputDevice) {
  TransformDefT transform;
  ReticleDefT reticle;

  Blueprint blueprint;
  blueprint.Write(&transform);
  blueprint.Write(&reticle);

  auto* entity_factory = registry_->Get<EntityFactory>();
  const Entity entity = entity_factory->Create(&blueprint);

  EXPECT_NE(entity, kNullEntity);

  // If there are no valid input devices, the reticle is given scale 0.
  registry_->Get<ReticleSystem>()->AdvanceFrame(kDeltaTime);
  const Sqt* sqt = registry_->Get<TransformSystem>()->GetSqt(entity);
  EXPECT_NEAR(sqt->scale.x, 0.f, kEpsilon);
  EXPECT_NEAR(sqt->scale.y, 0.f, kEpsilon);
  EXPECT_NEAR(sqt->scale.z, 0.f, kEpsilon);
}

TEST_F(ReticleSystemTest, BasicController) {
  Create3DofDevice();

  Blueprint blueprint1;
  {
    TransformDefT transform;
    blueprint1.Write(&transform);

    ReticleDefT reticle;
    reticle.ergo_angle_offset = 0.f;
    reticle.no_hit_distance = 2.f;
    blueprint1.Write(&reticle);
  }

  auto* entity_factory = registry_->Get<EntityFactory>();
  const Entity reticle = entity_factory->Create(&blueprint1);

  EXPECT_NE(reticle, kNullEntity);

  auto* reticle_system = registry_->Get<ReticleSystem>();
  EXPECT_EQ(reticle_system->GetActiveDevice(), InputManager::kController);

  // Build an Entity in the +X direction for later use.
  Blueprint blueprint2;
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(1.f, 0.f, 0.f);
    blueprint2.Write(&transform);

    CollisionDefT collision;
    blueprint2.Write(&collision);
  }

  const Entity target = entity_factory->Create(&blueprint2);
  EXPECT_NE(target, kNullEntity);

  registry_->Get<TransformSystem>()->SetAabb(
      target, Aabb(-mathfu::kOnes3f / 2.f, mathfu::kOnes3f / 2.f));

  // Update the input to have the controller pointing in the -Z direction,
  // missing the only collideable entity.
  auto* input = registry_->Get<InputManager>();
  input->UpdateRotation(InputManager::kController, mathfu::quat::identity);
  input->AdvanceFrame(kDeltaTime);

  reticle_system->AdvanceFrame(kDeltaTime);

  {
    const Ray collision_ray = reticle_system->GetCollisionRay();

    EXPECT_NEAR(collision_ray.direction.x, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.y, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.z, -1.f, kEpsilon);

    const Sqt* sqt = registry_->Get<TransformSystem>()->GetSqt(reticle);
    EXPECT_NEAR(sqt->translation.x, 0.f, kEpsilon);
    EXPECT_NEAR(sqt->translation.y, 0.f, kEpsilon);
    EXPECT_NEAR(sqt->translation.z, -2.f, kEpsilon);

    EXPECT_EQ(reticle_system->GetTarget(), kNullEntity);
  }

  // Now point the controller in the +X direction and expect it to hit the
  // "target" entity.
  input->UpdateRotation(InputManager::kController,
                        mathfu::quat::FromEulerAngles(
                            kDegreesToRadians * mathfu::vec3(0.f, -90.f, 0.f)));
  input->AdvanceFrame(kDeltaTime);

  ExpectReticleUpdateUniforms();
  reticle_system->AdvanceFrame(kDeltaTime);

  {
    const Ray collision_ray = reticle_system->GetCollisionRay();

    EXPECT_NEAR(collision_ray.direction.x, 1.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.y, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.z, 0.f, kEpsilon);

    const Sqt* sqt = registry_->Get<TransformSystem>()->GetSqt(reticle);
    EXPECT_NEAR(sqt->translation.x, 0.5f, kEpsilon);
    EXPECT_NEAR(sqt->translation.y, 0.f, kEpsilon);
    EXPECT_NEAR(sqt->translation.z, 0.f, kEpsilon);

    EXPECT_EQ(reticle_system->GetTarget(), target);
  }

  // Now point the controller in the +Z direction.
  input->UpdateRotation(InputManager::kController,
                        mathfu::quat::FromEulerAngles(
                            kDegreesToRadians * mathfu::vec3(0.f, 180.f, 0.f)));
  input->AdvanceFrame(kDeltaTime);

  ExpectReticleUpdateUniforms();
  reticle_system->AdvanceFrame(kDeltaTime);

  {
    const Ray collision_ray = reticle_system->GetCollisionRay();

    EXPECT_NEAR(collision_ray.direction.x, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.y, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.z, 1.f, kEpsilon);

    const Sqt* sqt = registry_->Get<TransformSystem>()->GetSqt(reticle);
    EXPECT_NEAR(sqt->translation.x, 0.f, kEpsilon);
    EXPECT_NEAR(sqt->translation.y, 0.f, kEpsilon);
    EXPECT_NEAR(sqt->translation.z, 2.f, kEpsilon);

    EXPECT_EQ(reticle_system->GetTarget(), kNullEntity);
  }
}

TEST_F(ReticleSystemTest, InputEvents) {
  Create3DofDevice();

  Blueprint blueprint1;
  {
    TransformDefT transform;
    blueprint1.Write(&transform);

    ReticleDefT reticle;
    reticle.ergo_angle_offset = 0.f;
    reticle.no_hit_distance = 2.f;
    blueprint1.Write(&reticle);
  }

  auto* entity_factory = registry_->Get<EntityFactory>();
  const Entity reticle = entity_factory->Create(&blueprint1);

  EXPECT_NE(reticle, kNullEntity);

  auto* reticle_system = registry_->Get<ReticleSystem>();
  EXPECT_EQ(reticle_system->GetActiveDevice(), InputManager::kController);

  // Setup handlers to track global hover and click state.
  auto* dispatcher = registry_->Get<Dispatcher>();
  Entity global_hovered = kNullEntity;
  Entity global_pressed = kNullEntity;
  int press_count = 0;
  int press_release_count = 0;

  auto global_start_connection =
      dispatcher->Connect([&global_hovered](const StartHoverEvent& event) {
        global_hovered = event.target;
      });
  auto global_end_connection =
      dispatcher->Connect([&global_hovered](const StopHoverEvent& event) {
        global_hovered = kNullEntity;
      });
  auto global_press_connection = dispatcher->Connect(
      [&global_pressed, &press_count](const ClickEvent& event) {
        global_pressed = event.target;
        ++press_count;
      });
  auto global_release_connection = dispatcher->Connect(
      [&global_pressed, &press_release_count](const ClickReleasedEvent& event) {
        global_pressed = kNullEntity;
        ++press_release_count;
      });

  // Build an Entity in the +X direction for later use.
  Blueprint blueprint2;
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(1.f, 0.f, 0.f);
    blueprint2.Write(&transform);

    CollisionDefT collision;
    blueprint2.Write(&collision);
  }

  const Entity target = entity_factory->Create(&blueprint2);
  EXPECT_NE(target, kNullEntity);

  registry_->Get<TransformSystem>()->SetAabb(
      target, Aabb(-mathfu::kOnes3f / 2.f, mathfu::kOnes3f / 2.f));

  // Setup handlers to track the target Entity's hover state.
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  bool local_hovered = false;
  bool local_pressed = false;
  bool local_pressed_and_released = false;

  auto local_start_connection = dispatcher_system->Connect(
      target,
      [&local_hovered](const StartHoverEvent& event) { local_hovered = true; });
  auto local_end_connection = dispatcher_system->Connect(
      target,
      [&local_hovered](const StopHoverEvent& event) { local_hovered = false; });
  auto local_press_connection = dispatcher_system->Connect(
      target,
      [&local_pressed](const ClickEvent& event) { local_pressed = true; });
  auto local_release_connection = dispatcher_system->Connect(
      target, [&local_pressed](const ClickReleasedEvent& event) {
        local_pressed = false;
      });
  auto local_click_release_connection = dispatcher_system->Connect(
      target,
      [&local_pressed_and_released](const ClickPressedAndReleasedEvent& event) {
        local_pressed_and_released = true;
      });

  // Update the input to have the controller pointing in the -Z direction,
  // missing the only collideable entity.
  auto* input = registry_->Get<InputManager>();
  input->UpdateRotation(InputManager::kController, mathfu::quat::identity);
  input->AdvanceFrame(kDeltaTime);

  reticle_system->AdvanceFrame(kDeltaTime);

  EXPECT_EQ(global_hovered, kNullEntity);
  EXPECT_FALSE(local_hovered);
  EXPECT_EQ(reticle_system->GetTarget(), kNullEntity);

  // Now point the controller in the +X direction and expect it to hit the
  // "target" entity.
  input->UpdateRotation(InputManager::kController,
                        mathfu::quat::FromEulerAngles(
                            kDegreesToRadians * mathfu::vec3(0.f, -90.f, 0.f)));
  input->AdvanceFrame(kDeltaTime);

  ExpectReticleUpdateUniforms();
  reticle_system->AdvanceFrame(kDeltaTime);

  EXPECT_EQ(global_hovered, target);
  EXPECT_TRUE(local_hovered);
  EXPECT_EQ(reticle_system->GetTarget(), target);

  // Press the controller button down.
  input->UpdateButton(InputManager::kController, InputManager::kPrimaryButton,
                      /* pressed */ true, false);
  input->AdvanceFrame(kDeltaTime);
  reticle_system->AdvanceFrame(kDeltaTime);

  EXPECT_EQ(global_hovered, target);
  EXPECT_TRUE(local_hovered);
  EXPECT_EQ(reticle_system->GetTarget(), target);

  EXPECT_EQ(global_pressed, target);
  EXPECT_TRUE(local_pressed);
  EXPECT_EQ(press_count, 1);

  // Release the controller button.
  input->UpdateButton(InputManager::kController, InputManager::kPrimaryButton,
                      /* pressed */ false, false);
  input->AdvanceFrame(kDeltaTime);
  reticle_system->AdvanceFrame(kDeltaTime);

  EXPECT_EQ(global_hovered, target);
  EXPECT_TRUE(local_hovered);
  EXPECT_EQ(reticle_system->GetTarget(), target);

  EXPECT_EQ(global_pressed, kNullEntity);
  EXPECT_FALSE(local_pressed);
  EXPECT_TRUE(local_pressed_and_released);
  EXPECT_EQ(press_release_count, 1);

  // Now point the controller back in the -Z direction to ensure stop hover
  // events are dispatched.
  input->UpdateRotation(InputManager::kController, mathfu::quat::identity);
  input->AdvanceFrame(kDeltaTime);

  ExpectReticleUpdateUniforms();
  reticle_system->AdvanceFrame(kDeltaTime);

  EXPECT_EQ(global_hovered, kNullEntity);
  EXPECT_FALSE(local_hovered);
  EXPECT_EQ(reticle_system->GetTarget(), kNullEntity);
}

TEST_F(ReticleSystemTest, ReticleBehaviour) {
  Create3DofDevice();

  Blueprint blueprint1;
  {
    TransformDefT transform;
    blueprint1.Write(&transform);

    ReticleDefT reticle;
    reticle.ergo_angle_offset = 0.f;
    reticle.no_hit_distance = 2.f;
    blueprint1.Write(&reticle);
  }

  auto* entity_factory = registry_->Get<EntityFactory>();
  const Entity reticle = entity_factory->Create(&blueprint1);

  EXPECT_NE(reticle, kNullEntity);

  auto* reticle_system = registry_->Get<ReticleSystem>();
  EXPECT_EQ(reticle_system->GetActiveDevice(), InputManager::kController);

  // Setup handlers to track global hover state.
  auto* dispatcher = registry_->Get<Dispatcher>();
  Entity global_hovered = kNullEntity;

  auto global_start_connection =
      dispatcher->Connect([&global_hovered](const StartHoverEvent& event) {
        global_hovered = event.target;
      });
  auto global_end_connection =
      dispatcher->Connect([&global_hovered](const StopHoverEvent& event) {
        global_hovered = kNullEntity;
      });

  // Build an Entity with no collision body that will be  handling events for
  // it's children.
  Blueprint blueprint2;
  {
    TransformDefT transform;
    blueprint2.Write(&transform);

    ReticleBehaviourDefT behaviour;
    behaviour.collision_behaviour = ReticleCollisionBehaviour_HandleDescendants;
    blueprint2.Write(&behaviour);
  }

  const Entity parent = entity_factory->Create(&blueprint2);
  EXPECT_NE(parent, kNullEntity);

  // Build a collidable Entity that will be forwarding events to it's parent.
  // Give it a hover-start dead zone.
  Blueprint blueprint3;
  {
    // Start this Entity completely outside of ray collision, including the
    // dead zone.
    TransformDefT transform;
    transform.position = mathfu::vec3(3.f, 0.f, -2.f);
    blueprint3.Write(&transform);

    CollisionDefT collision;
    blueprint3.Write(&collision);

    ReticleBehaviourDefT behaviour;
    behaviour.hover_start_dead_zone = mathfu::vec3(1.f, 1.f, 1.f);
    behaviour.collision_behaviour = ReticleCollisionBehaviour_FindAncestor;
    blueprint3.Write(&behaviour);
  }

  const Entity child = entity_factory->Create(&blueprint3);
  EXPECT_NE(child, kNullEntity);

  auto* transform_system = registry_->Get<TransformSystem>();
  transform_system->SetAabb(
      child, Aabb(-2.f * mathfu::kOnes3f, 2.f * mathfu::kOnes3f));
  transform_system->AddChild(parent, child);

  // Update the input to have the controller pointing in the -Z direction,
  // missing the only collideable entity.
  auto* input = registry_->Get<InputManager>();
  input->UpdateRotation(InputManager::kController, mathfu::quat::identity);
  input->AdvanceFrame(kDeltaTime);

  reticle_system->AdvanceFrame(kDeltaTime);

  EXPECT_EQ(global_hovered, kNullEntity);
  EXPECT_EQ(reticle_system->GetTarget(), kNullEntity);

  // Move the child Entity into collision, but have it be in the dead zone, so
  // nothing happens.
  transform_system->SetSqt(child, Sqt(mathfu::vec3(1.5f, 0.f, -2.f),
                                      mathfu::quat::identity, mathfu::kOnes3f));

  reticle_system->AdvanceFrame(kDeltaTime);

  EXPECT_EQ(global_hovered, kNullEntity);
  EXPECT_EQ(reticle_system->GetTarget(), kNullEntity);

  // Move it further into collision past the dead zone, then ensure events are
  // dispatched to the parent.
  transform_system->SetSqt(child, Sqt(mathfu::vec3(0.f, 0.f, -2.f),
                                      mathfu::quat::identity, mathfu::kOnes3f));

  ExpectReticleUpdateUniforms();
  reticle_system->AdvanceFrame(kDeltaTime);

  EXPECT_EQ(global_hovered, parent);
  EXPECT_EQ(reticle_system->GetTarget(), parent);

  // Move it back into the dead zone, and ensure that nothing changes since the
  // dead zone only affects hover start.
  transform_system->SetSqt(child, Sqt(mathfu::vec3(1.5f, 0.f, -2.f),
                                      mathfu::quat::identity, mathfu::kOnes3f));

  reticle_system->AdvanceFrame(kDeltaTime);

  EXPECT_EQ(global_hovered, parent);
  EXPECT_EQ(reticle_system->GetTarget(), parent);

  // Move it further all the way out of collision.
  transform_system->SetSqt(child, Sqt(mathfu::vec3(3.f, 0.f, -2.f),
                                      mathfu::quat::identity, mathfu::kOnes3f));

  ExpectReticleUpdateUniforms();
  reticle_system->AdvanceFrame(kDeltaTime);

  EXPECT_EQ(global_hovered, kNullEntity);
  EXPECT_EQ(reticle_system->GetTarget(), kNullEntity);
}

TEST_F(ReticleSystemTest, Locking) {
  Create3DofDevice();

  Blueprint blueprint1;
  {
    TransformDefT transform;
    blueprint1.Write(&transform);

    ReticleDefT reticle;
    reticle.ergo_angle_offset = 0.f;
    reticle.no_hit_distance = 2.f;
    blueprint1.Write(&reticle);
  }

  auto* entity_factory = registry_->Get<EntityFactory>();
  const Entity reticle = entity_factory->Create(&blueprint1);

  EXPECT_NE(reticle, kNullEntity);

  auto* reticle_system = registry_->Get<ReticleSystem>();
  EXPECT_EQ(reticle_system->GetActiveDevice(), InputManager::kController);

  // Build an Entity in the +X direction for later use.
  Blueprint blueprint2;
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(1.f, 0.f, 0.f);
    blueprint2.Write(&transform);

    CollisionDefT collision;
    blueprint2.Write(&collision);
  }

  const Entity targetX = entity_factory->Create(&blueprint2);
  EXPECT_NE(targetX, kNullEntity);

  auto* transform_system = registry_->Get<TransformSystem>();
  transform_system->SetAabb(
      targetX, Aabb(-mathfu::kOnes3f / 2.f, mathfu::kOnes3f / 2.f));

  // Build a second Entity in the -Z direction for later use.
  Blueprint blueprint3;
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(0.f, 0.f, -2.f);
    blueprint3.Write(&transform);

    CollisionDefT collision;
    blueprint3.Write(&collision);
  }

  const Entity targetZ = entity_factory->Create(&blueprint3);
  EXPECT_NE(targetZ, kNullEntity);

  registry_->Get<TransformSystem>()->SetAabb(
      targetZ, Aabb(-mathfu::kOnes3f / 2.f, mathfu::kOnes3f / 2.f));

  // Update the input to have the controller pointing in the -Z direction,
  // hitting targetZ.
  auto* input = registry_->Get<InputManager>();
  input->UpdateRotation(InputManager::kController, mathfu::quat::identity);
  input->AdvanceFrame(kDeltaTime);

  ExpectReticleUpdateUniforms();
  reticle_system->LockOn(targetZ, mathfu::kOnes3f * 0.3f);
  reticle_system->AdvanceFrame(kDeltaTime);

  {
    const Ray collision_ray = reticle_system->GetCollisionRay();

    EXPECT_NEAR(collision_ray.direction.x, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.y, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.z, -1.f, kEpsilon);

    const Sqt* sqt = registry_->Get<TransformSystem>()->GetSqt(reticle);
    // Expect this to be targetZ position + (0.3,0.3,0.3).
    EXPECT_NEAR(sqt->translation.x, 0.3f, kEpsilon);
    EXPECT_NEAR(sqt->translation.y, 0.3f, kEpsilon);
    EXPECT_NEAR(sqt->translation.z, -1.7f, kEpsilon);

    EXPECT_EQ(reticle_system->GetTarget(), targetZ);
  }

  // Now point the controller in the +X direction, but expect it to still hit
  // the locked target.
  input->UpdateRotation(InputManager::kController,
                        mathfu::quat::FromEulerAngles(
                            kDegreesToRadians * mathfu::vec3(0.f, -90.f, 0.f)));
  input->AdvanceFrame(kDeltaTime);

  reticle_system->AdvanceFrame(kDeltaTime);

  {
    const Ray collision_ray = reticle_system->GetCollisionRay();

    EXPECT_NEAR(collision_ray.direction.x, 1.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.y, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.z, 0.f, kEpsilon);

    const Sqt* sqt = registry_->Get<TransformSystem>()->GetSqt(reticle);
    // Expect this to be targetZ position + (0.3,0.3,0.3).
    EXPECT_NEAR(sqt->translation.x, 0.3f, kEpsilon);
    EXPECT_NEAR(sqt->translation.y, 0.3f, kEpsilon);
    EXPECT_NEAR(sqt->translation.z, -1.7f, kEpsilon);

    EXPECT_EQ(reticle_system->GetTarget(), targetZ);
  }

  // Now point the controller back to -z, and put targetX in the way of targetZ.
  Sqt sqt;
  sqt.translation = mathfu::vec3(0.0f, 0.0f, -1.0f);
  transform_system->SetSqt(targetX, sqt);
  input->UpdateRotation(InputManager::kController, mathfu::quat::identity);
  input->AdvanceFrame(kDeltaTime);

  reticle_system->AdvanceFrame(kDeltaTime);

  {
    const Ray collision_ray = reticle_system->GetCollisionRay();

    EXPECT_NEAR(collision_ray.direction.x, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.y, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.z, -1.f, kEpsilon);

    const Sqt* sqt = registry_->Get<TransformSystem>()->GetSqt(reticle);
    // Expect this to be targetZ position + (0.3,0.3,0.3).
    EXPECT_NEAR(sqt->translation.x, 0.3f, kEpsilon);
    EXPECT_NEAR(sqt->translation.y, 0.3f, kEpsilon);
    EXPECT_NEAR(sqt->translation.z, -1.7f, kEpsilon);

    EXPECT_EQ(reticle_system->GetTarget(), targetZ);
  }

  // Now release the lock and expect the target to be targetX
  reticle_system->LockOn(kNullEntity, mathfu::kZeros3f);
  input->AdvanceFrame(kDeltaTime);

  reticle_system->AdvanceFrame(kDeltaTime);

  {
    const Ray collision_ray = reticle_system->GetCollisionRay();

    EXPECT_NEAR(collision_ray.direction.x, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.y, 0.f, kEpsilon);
    EXPECT_NEAR(collision_ray.direction.z, -1.f, kEpsilon);

    const Sqt* sqt = registry_->Get<TransformSystem>()->GetSqt(reticle);
    EXPECT_NEAR(sqt->translation.x, 0.0f, kEpsilon);
    EXPECT_NEAR(sqt->translation.y, 0.0f, kEpsilon);
    EXPECT_NEAR(sqt->translation.z, -0.5f, kEpsilon);

    EXPECT_EQ(reticle_system->GetTarget(), targetX);
  }
}
}  // namespace
}  // namespace lull
