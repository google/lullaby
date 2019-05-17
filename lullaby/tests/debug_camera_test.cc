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

#include "lullaby/modules/debug/debug_camera.h"
#include "gtest/gtest.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/time.h"
#include "lullaby/tests/mathfu_matchers.h"

namespace lull {
namespace {

using ::testing::FloatEq;
using ::testing::FloatNear;

constexpr Clock::duration kDeltaTime = std::chrono::milliseconds(17);
constexpr float kTestEpsilon = 1e-5f;

class DebugCameraTest : public ::testing::Test {
 public:
  DebugCameraTest() {
    registry_.Create<Dispatcher>();
    input_manager_ = registry_.Create<InputManager>();
    debug_camera_.reset(new DebugCamera(&registry_));
  }

 protected:
  Registry registry_;
  InputManager* input_manager_;
  std::unique_ptr<DebugCamera> debug_camera_;
};

TEST_F(DebugCameraTest, StartStopAndCameraMovement) {
  const auto device = InputManager::kController;

  DeviceProfile profile;
  profile.touchpads.resize(1);
  profile.touchpads[0].has_gestures = true;
  profile.rotation_dof = DeviceProfile::kRealDof;
  input_manager_->ConnectDevice(device, profile);

  input_manager_->AdvanceFrame(kDeltaTime);

  mathfu::vec3 start_position = debug_camera_->GetTranslation();

  debug_camera_->StartDebugMode();

  // Subtest #1:  Enter camera debug mode, simulate controller movement,
  // check for camera movement.
  input_manager_->UpdateTouch(device, mathfu::vec2(0, 0), true);
  input_manager_->UpdateGesture(device, InputManager::kPrimaryTouchpadId,
                                InputManager::GestureType::kFling,
                                InputManager::GestureDirection::kUp,
                                mathfu::vec2(0, 1), mathfu::vec2(0, 1));
  input_manager_->AdvanceFrame(kDeltaTime);
  debug_camera_->AdvanceFrame(kDeltaTime);

  mathfu::vec3 fling_while_debugging_position = debug_camera_->GetTranslation();

  // We set the movement rate to 1 unit/sec, so our distance moved should be
  // equal to delta time.
  EXPECT_THAT(
      mathfu::vec3::Distance(start_position, fling_while_debugging_position),
      FloatNear(lull::SecondsFromDuration(kDeltaTime), kTestEpsilon));

  // Subtest #2:  Leave camera debug mode, check that original camera position
  // is restored.
  debug_camera_->StopDebugMode();

  mathfu::vec3 after_debugging_position = debug_camera_->GetTranslation();

  EXPECT_THAT(mathfu::vec3::Distance(start_position, after_debugging_position),
              FloatEq(0));
}

}  // namespace
}  // namespace lull
