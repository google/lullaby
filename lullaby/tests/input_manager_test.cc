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

#include "lullaby/modules/input/input_manager.h"

#include "gtest/gtest.h"
#include "lullaby/util/bits.h"
#include "lullaby/util/clock.h"
#include "lullaby/tests/mathfu_matchers.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

using testing::NearMathfu;

constexpr float kEpsilon = 1e-5f;

const Clock::duration kDeltaTime = std::chrono::milliseconds(17);
const Clock::duration kLongPressTime = std::chrono::milliseconds(500);

TEST(InputManagerDeathTest, NoConnections) {
  InputManager input;
  EXPECT_FALSE(input.IsConnected(InputManager::kHmd));
  EXPECT_FALSE(input.IsConnected(InputManager::kMouse));
  EXPECT_FALSE(input.IsConnected(InputManager::kKeyboard));
  EXPECT_FALSE(input.IsConnected(InputManager::kController));
  EXPECT_FALSE(input.IsConnected(InputManager::kController2));
  EXPECT_FALSE(input.IsConnected(InputManager::kHand));
  PORT_EXPECT_DEBUG_DEATH(input.IsConnected(InputManager::kMaxNumDeviceTypes),
                          "");
}

TEST(InputManager, DeviceNames) {
  InputManager input;
  EXPECT_NE(InputManager::GetDeviceName(InputManager::kMouse), "");
  EXPECT_NE(InputManager::GetDeviceName(InputManager::kKeyboard), "");
  EXPECT_NE(InputManager::GetDeviceName(InputManager::kController), "");
  EXPECT_NE(InputManager::GetDeviceName(InputManager::kController2), "");
  EXPECT_NE(InputManager::GetDeviceName(InputManager::kHand), "");
}

TEST(InputManagerDeathTest, InvalidDevice_Profile) {
  InputManager input;
  const auto device = InputManager::kMaxNumDeviceTypes;
  PORT_EXPECT_DEBUG_DEATH(input.ConnectDevice(device, DeviceProfile()), "");
  PORT_EXPECT_DEBUG_DEATH(input.DisconnectDevice(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.IsConnected(device), "");
}

TEST(InputManagerDeathTest, InvalidDevice_Get) {
  InputManager input;
  const auto device = InputManager::kMaxNumDeviceTypes;
  PORT_EXPECT_DEBUG_DEATH(input.GetKeyState(device, ""), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetPressedKeys(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetButtonState(device, 0), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetButtonPressedDuration(device, 0), "");
  PORT_EXPECT_DEBUG_DEATH(
      input.GetJoystickValue(device, InputManager::kLeftJoystick), "");
  PORT_EXPECT_DEBUG_DEATH(
      input.GetJoystickDelta(device, InputManager::kLeftJoystick), "");
  PORT_EXPECT_DEBUG_DEATH(input.IsValidTouch(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchState(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchLocation(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchDelta(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchVelocity(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchGestureDirection(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofPosition(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofDelta(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofRotation(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofAngularDelta(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofWorldFromObjectMatrix(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetScrollDelta(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetEyeFromHead(device, 0), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetScreenFromEye(device, 0), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetEyeFOV(device, 0), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetKeyState(device, ""), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetButtonState(device, 0), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetBatteryCharge(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetBatteryState(device), "");
}

TEST(InputManagerDeathTest, InvalidProfile_Get) {
  InputManager input;
  const auto device = InputManager::kController;
  DeviceProfile profile;
  input.ConnectDevice(device, profile);

  PORT_EXPECT_DEBUG_DEATH(input.GetButtonPressedDuration(device, 2), "");
  PORT_EXPECT_DEBUG_DEATH(
      input.GetJoystickValue(device, InputManager::kDirectionalPad), "");
  PORT_EXPECT_DEBUG_DEATH(
      input.GetJoystickDelta(device, InputManager::kDirectionalPad), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchLocation(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchDelta(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchVelocity(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchGestureDirection(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetEyeFromHead(device, 2), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetScreenFromEye(device, 2), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetEyeFOV(device, 2), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofPosition(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofDelta(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofRotation(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofAngularDelta(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofWorldFromObjectMatrix(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetScrollDelta(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetBatteryCharge(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetBatteryState(device), "");
}

TEST(InputManagerDeathTest, InvalidDevice_Update) {
  InputManager input;
  const auto device = InputManager::kMaxNumDeviceTypes;
  PORT_EXPECT_DEBUG_DEATH(input.UpdateKey(device, " ", true), "");
  PORT_EXPECT_DEBUG_DEATH(input.KeyPressed(device, " "), "");
  PORT_EXPECT_DEBUG_DEATH(input.UpdateButton(device, 0, true, true), "");
  PORT_EXPECT_DEBUG_DEATH(
      input.UpdateJoystick(device, InputManager::kLeftJoystick,
                           mathfu::kZeros2f),
      "");
  PORT_EXPECT_DEBUG_DEATH(
      input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                        mathfu::kZeros2f, true),
      "");
  PORT_EXPECT_DEBUG_DEATH(
      input.UpdateGesture(device, InputManager::kPrimaryTouchpadId,
                          InputManager::GestureType::kNone,
                          InputManager::GestureDirection::kNone,
                          mathfu::kZeros2f, mathfu::kZeros2f),
      "");
  PORT_EXPECT_DEBUG_DEATH(input.UpdateScroll(device, 0), "");
  PORT_EXPECT_DEBUG_DEATH(input.UpdatePosition(device, mathfu::kZeros3f), "");
  PORT_EXPECT_DEBUG_DEATH(input.UpdateRotation(device, mathfu::quat()), "");
  PORT_EXPECT_DEBUG_DEATH(input.UpdateEye(device, 0, mathfu::mat4(),
                                          mathfu::mat4(), mathfu::rectf()),
                          "");
  PORT_EXPECT_DEBUG_DEATH(
      input.UpdateBattery(device, InputManager::BatteryState::kUnknown, 0), "");
}

TEST(InputManagerDeathTest, InvalidProfile_Update) {
  InputManager input;
  const auto device = InputManager::kController;
  DeviceProfile profile;
  input.ConnectDevice(device, profile);

  PORT_EXPECT_DEBUG_DEATH(input.UpdateKey(device, " ", true), "");
  PORT_EXPECT_DEBUG_DEATH(input.UpdateButton(device, 0, true, true), "");
  PORT_EXPECT_DEBUG_DEATH(
      input.UpdateJoystick(device, InputManager::kLeftJoystick,
                           mathfu::kZeros2f),
      "");
  PORT_EXPECT_DEBUG_DEATH(
      input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                        mathfu::kZeros2f, true),
      "");
  PORT_EXPECT_DEBUG_DEATH(
      input.UpdateGesture(device, InputManager::kPrimaryTouchpadId,
                          InputManager::GestureType::kNone,
                          InputManager::GestureDirection::kNone,
                          mathfu::kZeros2f, mathfu::kZeros2f),
      "");
  PORT_EXPECT_DEBUG_DEATH(input.UpdateScroll(device, 0), "");
  PORT_EXPECT_DEBUG_DEATH(input.UpdatePosition(device, mathfu::kZeros3f), "");
  PORT_EXPECT_DEBUG_DEATH(input.UpdateRotation(device, mathfu::quat()), "");
  PORT_EXPECT_DEBUG_DEATH(input.UpdateEye(device, 0, mathfu::mat4(),
                                          mathfu::mat4(), mathfu::rectf()),
                          "");
  PORT_EXPECT_DEBUG_DEATH(
      input.UpdateBattery(device, InputManager::BatteryState::kUnknown, 0), "");
}

TEST(InputManager, DeviceState) {
  InputManager input;
  const auto button = InputManager::kPrimaryButton;
  const auto device = InputManager::kController;
  const auto joystick = InputManager::kLeftJoystick;
  const auto eye = 0;
  const std::string key = "a";

  DeviceProfile profile;
  profile.buttons.resize(1);
  input.ConnectDevice(device, profile);
  EXPECT_EQ(true, input.IsConnected(device));
  EXPECT_FALSE(input.HasPositionDof(device));
  EXPECT_FALSE(input.HasRotationDof(device));
  EXPECT_FALSE(input.HasTouchpad(device));
  EXPECT_FALSE(input.HasJoystick(device, joystick));
  EXPECT_FALSE(input.HasScroll(device));
  EXPECT_TRUE(input.HasButton(device, button));
  EXPECT_EQ(input.GetNumButtons(device), 1u);
  EXPECT_FALSE(input.HasEye(device, eye));
  EXPECT_EQ(input.GetNumEyes(device), 0u);
  EXPECT_FALSE(input.HasBattery(device));

  input.DisconnectDevice(device);

  profile.rotation_dof = DeviceProfile::kRealDof;
  profile.position_dof = DeviceProfile::kRealDof;
  profile.touchpads.resize(1);
  profile.scroll_wheels.resize(1);
  profile.battery.emplace();
  profile.joysticks.resize(2);
  profile.buttons.resize(3);
  profile.eyes.resize(2);
  input.ConnectDevice(device, profile);
  EXPECT_EQ(true, input.IsConnected(device));
  EXPECT_TRUE(input.HasPositionDof(device));
  EXPECT_TRUE(input.HasRotationDof(device));
  EXPECT_TRUE(input.HasTouchpad(device));
  EXPECT_TRUE(input.HasJoystick(device, joystick));
  EXPECT_TRUE(input.HasScroll(device));
  EXPECT_TRUE(input.HasButton(device, button));
  EXPECT_EQ(input.GetNumButtons(device), 3u);
  EXPECT_TRUE(input.HasEye(device, eye));
  EXPECT_EQ(input.GetNumEyes(device), 2u);
  EXPECT_TRUE(input.HasBattery(device));
  input.DisconnectDevice(device);
}

TEST(InputManager, DeviceParams_Legacy) {
  InputManager input;
  const auto button = InputManager::kPrimaryButton;
  const auto device = InputManager::kController;
  const auto joystick = InputManager::kLeftJoystick;
  const auto eye = 0;
  const std::string key = "a";

  InputManager::DeviceParams params;
  params.num_buttons = 1;
  input.ConnectDevice(device, params);
  EXPECT_EQ(true, input.IsConnected(device));
  EXPECT_FALSE(input.HasPositionDof(device));
  EXPECT_FALSE(input.HasRotationDof(device));
  EXPECT_FALSE(input.HasTouchpad(device));
  EXPECT_FALSE(input.HasJoystick(device, joystick));
  EXPECT_FALSE(input.HasScroll(device));
  EXPECT_TRUE(input.HasButton(device, button));
  EXPECT_EQ(input.GetNumButtons(device), 1u);
  EXPECT_FALSE(input.HasEye(device, eye));
  EXPECT_EQ(input.GetNumEyes(device), 0u);
  EXPECT_FALSE(input.HasBattery(device));

  input.DisconnectDevice(device);

  params.has_position_dof = true;
  params.has_rotation_dof = true;
  params.has_touchpad = true;
  params.has_scroll = true;
  params.has_battery = true;
  params.num_joysticks = 2;
  params.num_buttons = 3;
  params.num_eyes = 2;
  input.ConnectDevice(device, params);
  EXPECT_EQ(true, input.IsConnected(device));
  EXPECT_TRUE(input.HasPositionDof(device));
  EXPECT_TRUE(input.HasRotationDof(device));
  EXPECT_TRUE(input.HasTouchpad(device));
  EXPECT_TRUE(input.HasJoystick(device, joystick));
  EXPECT_TRUE(input.HasScroll(device));
  EXPECT_TRUE(input.HasButton(device, button));
  EXPECT_EQ(input.GetNumButtons(device), 3u);
  EXPECT_TRUE(input.HasEye(device, eye));
  EXPECT_EQ(input.GetNumEyes(device), 2u);
  EXPECT_TRUE(input.HasBattery(device));
  input.DisconnectDevice(device);
}

TEST(InputManagerDeathTest, ButtonState) {
  InputManager input;
  const auto button = InputManager::kPrimaryButton;
  const auto device = InputManager::kController;

  PORT_EXPECT_DEBUG_DEATH(input.GetButtonState(device, button), "");

  DeviceProfile profile;
  profile.buttons.resize(1);
  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  PORT_EXPECT_DEBUG_DEATH(input.GetButtonState(device, button + 1), "");

  auto state = input.GetButtonState(device, button);
  EXPECT_TRUE(CheckBit(state, InputManager::kReleased));
  EXPECT_TRUE(!CheckBit(state, InputManager::kPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustReleased));
  EXPECT_TRUE(!CheckBit(state, InputManager::kRepeat));
  EXPECT_TRUE(!CheckBit(state, InputManager::kLongPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustLongPressed));

  input.UpdateButton(device, button, true, false);
  input.AdvanceFrame(kDeltaTime);

  state = input.GetButtonState(device, button);
  EXPECT_TRUE(!CheckBit(state, InputManager::kReleased));
  EXPECT_TRUE(CheckBit(state, InputManager::kPressed));
  EXPECT_TRUE(CheckBit(state, InputManager::kJustPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustReleased));
  EXPECT_TRUE(!CheckBit(state, InputManager::kRepeat));
  EXPECT_TRUE(!CheckBit(state, InputManager::kLongPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustLongPressed));

  input.UpdateButton(device, button, true, false);
  input.AdvanceFrame(kDeltaTime);

  state = input.GetButtonState(device, button);
  EXPECT_TRUE(!CheckBit(state, InputManager::kReleased));
  EXPECT_TRUE(CheckBit(state, InputManager::kPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustReleased));
  EXPECT_TRUE(!CheckBit(state, InputManager::kRepeat));
  EXPECT_TRUE(!CheckBit(state, InputManager::kLongPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustLongPressed));

  input.UpdateButton(device, button, true, false);
  input.AdvanceFrame(kLongPressTime);

  state = input.GetButtonState(device, button);
  EXPECT_TRUE(!CheckBit(state, InputManager::kReleased));
  EXPECT_TRUE(CheckBit(state, InputManager::kPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustReleased));
  EXPECT_TRUE(!CheckBit(state, InputManager::kRepeat));
  EXPECT_TRUE(CheckBit(state, InputManager::kLongPressed));
  EXPECT_TRUE(CheckBit(state, InputManager::kJustLongPressed));

  input.UpdateButton(device, button, true, false);
  input.AdvanceFrame(kDeltaTime);

  state = input.GetButtonState(device, button);
  EXPECT_TRUE(!CheckBit(state, InputManager::kReleased));
  EXPECT_TRUE(CheckBit(state, InputManager::kPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustReleased));
  EXPECT_TRUE(!CheckBit(state, InputManager::kRepeat));
  EXPECT_TRUE(CheckBit(state, InputManager::kLongPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustLongPressed));

  input.UpdateButton(device, button, true, true);
  input.AdvanceFrame(kDeltaTime);

  state = input.GetButtonState(device, button);
  EXPECT_TRUE(!CheckBit(state, InputManager::kReleased));
  EXPECT_TRUE(CheckBit(state, InputManager::kPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustReleased));
  EXPECT_TRUE(CheckBit(state, InputManager::kRepeat));
  EXPECT_TRUE(CheckBit(state, InputManager::kLongPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustLongPressed));

  input.UpdateButton(device, button, false, false);
  input.AdvanceFrame(kDeltaTime);

  state = input.GetButtonState(device, button);
  EXPECT_TRUE(CheckBit(state, InputManager::kReleased));
  EXPECT_TRUE(!CheckBit(state, InputManager::kPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustPressed));
  EXPECT_TRUE(CheckBit(state, InputManager::kJustReleased));
  EXPECT_TRUE(!CheckBit(state, InputManager::kRepeat));
  EXPECT_TRUE(CheckBit(state, InputManager::kLongPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustLongPressed));

  input.UpdateButton(device, button, false, false);
  input.AdvanceFrame(kDeltaTime);

  state = input.GetButtonState(device, button);
  EXPECT_TRUE(CheckBit(state, InputManager::kReleased));
  EXPECT_TRUE(!CheckBit(state, InputManager::kPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustReleased));
  EXPECT_TRUE(!CheckBit(state, InputManager::kRepeat));
  EXPECT_TRUE(!CheckBit(state, InputManager::kLongPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustLongPressed));

  input.DisconnectDevice(device);
  EXPECT_TRUE(!input.IsConnected(device));
}

TEST(InputManager, JustLongPressedCornerCase) {
  // Test a corner case where a JustPressed and JustLongPressed need to both be
  // true.
  InputManager input;
  const auto button = InputManager::kPrimaryButton;
  const auto device = InputManager::kController;

  PORT_EXPECT_DEBUG_DEATH(input.GetButtonState(device, button), "");

  DeviceProfile profile;
  profile.buttons.resize(1);
  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  PORT_EXPECT_DEBUG_DEATH(input.GetButtonState(device, button + 1), "");

  auto state = input.GetButtonState(device, button);
  EXPECT_TRUE(CheckBit(state, InputManager::kReleased));
  EXPECT_TRUE(!CheckBit(state, InputManager::kPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustReleased));
  EXPECT_TRUE(!CheckBit(state, InputManager::kRepeat));
  EXPECT_TRUE(!CheckBit(state, InputManager::kLongPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustLongPressed));

  input.UpdateButton(device, button, false, false);
  input.AdvanceFrame(kLongPressTime);

  input.UpdateButton(device, button, true, false);
  input.AdvanceFrame(kLongPressTime);

  state = input.GetButtonState(device, button);
  EXPECT_TRUE(!CheckBit(state, InputManager::kReleased));
  EXPECT_TRUE(CheckBit(state, InputManager::kPressed));
  EXPECT_TRUE(CheckBit(state, InputManager::kJustPressed));
  EXPECT_TRUE(!CheckBit(state, InputManager::kJustReleased));
  EXPECT_TRUE(!CheckBit(state, InputManager::kRepeat));
  EXPECT_TRUE(CheckBit(state, InputManager::kLongPressed));
  EXPECT_TRUE(CheckBit(state, InputManager::kJustLongPressed));

  input.DisconnectDevice(device);
  EXPECT_TRUE(!input.IsConnected(device));
}

TEST(InputManagerDeathTest, ButtonPressedDuration) {
  InputManager input;
  const auto button = InputManager::kPrimaryButton;
  const auto device = InputManager::kController;

  PORT_EXPECT_DEBUG_DEATH(input.GetButtonState(device, button), "");

  DeviceProfile profile;
  profile.buttons.resize(1);
  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  PORT_EXPECT_DEBUG_DEATH(input.GetButtonState(device, button + 1), "");

  input.AdvanceFrame(kDeltaTime);
  EXPECT_EQ(input.GetButtonPressedDuration(device, button).count(), 0);

  input.UpdateButton(device, button, true, false);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetButtonPressedDuration(device, button).count(),
            kDeltaTime.count());

  input.UpdateButton(device, button, true, false);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetButtonPressedDuration(device, button).count(),
            kDeltaTime.count() * 2);

  input.UpdateButton(device, button, true, false);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetButtonPressedDuration(device, button).count(),
            kDeltaTime.count() * 3);

  input.UpdateButton(device, button, false, false);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetButtonPressedDuration(device, button).count(),
            kDeltaTime.count() * 3);

  input.UpdateButton(device, button, false, false);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetButtonPressedDuration(device, button).count(), 0);

  input.DisconnectDevice(device);
  EXPECT_TRUE(!input.IsConnected(device));
}

TEST(InputManagerDeathTest, KeyState) {
  InputManager input;
  const std::string key1 = "a";
  const std::string key2 = " ";
  const auto device = InputManager::kKeyboard;

  PORT_EXPECT_DEBUG_DEATH(input.GetKeyState(device, key1), "");

  DeviceProfile profile;

  input.ConnectDevice(InputManager::kController, profile);
  PORT_EXPECT_DEBUG_DEATH(input.GetKeyState(InputManager::kController, key1),
                          "");
  input.DisconnectDevice(InputManager::kController);

  input.ConnectDevice(device, profile);
  EXPECT_EQ(true, input.IsConnected(device));

  auto keys = input.GetPressedKeys(device);
  input.AdvanceFrame(kDeltaTime);
  EXPECT_EQ(keys.size(), 0u);

  input.KeyPressed(device, key1);
  input.AdvanceFrame(kDeltaTime);
  keys = input.GetPressedKeys(device);
  EXPECT_EQ(keys.size(), 1u);
  EXPECT_EQ(keys[0], key1);

  input.KeyPressed(device, key1);
  input.KeyPressed(device, key2);
  input.AdvanceFrame(kDeltaTime);
  keys = input.GetPressedKeys(device);
  EXPECT_EQ(keys.size(), 2u);
  EXPECT_EQ(keys[0], key1);
  EXPECT_EQ(keys[1], key2);

  input.AdvanceFrame(kDeltaTime);
  keys = input.GetPressedKeys(device);
  EXPECT_EQ(keys.size(), 0u);

  // Keyboard support for UpdateKey / GetKeyState not yet implemented.
  PORT_EXPECT_DEBUG_DEATH(input.UpdateKey(device, key1, true), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetKeyState(device, key1), "");

  input.DisconnectDevice(device);
  EXPECT_TRUE(!input.IsConnected(device));
}

TEST(InputManagerDeathTest, DegreesOfFreedom) {
  InputManager input;
  const auto device = InputManager::kHmd;
  const float kEpsilon = 0.00001f;

  PORT_EXPECT_DEBUG_DEATH(input.GetDofPosition(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofDelta(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofRotation(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofAngularDelta(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofWorldFromObjectMatrix(device), "");

  DeviceProfile profile;
  profile.rotation_dof = DeviceProfile::kUnavailableDof;
  profile.position_dof = DeviceProfile::kUnavailableDof;
  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  PORT_EXPECT_DEBUG_DEATH(input.UpdatePosition(device, mathfu::kZeros3f), "");
  PORT_EXPECT_DEBUG_DEATH(input.UpdateRotation(device, mathfu::quat::identity),
                          "");

  PORT_EXPECT_DEBUG_DEATH(input.GetDofPosition(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofDelta(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofRotation(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofAngularDelta(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetDofWorldFromObjectMatrix(device), "");

  const mathfu::vec3 eulers = mathfu::vec3(3.14159f, 0, 0);
  const mathfu::vec3 pos(1, 2, 3);
  const mathfu::quat half_rot = mathfu::quat::FromEulerAngles(eulers / 2.0f);
  const mathfu::quat rot = mathfu::quat::FromEulerAngles(eulers);

  input.DisconnectDevice(device);
  EXPECT_TRUE(!input.IsConnected(device));

  PORT_EXPECT_DEBUG_DEATH(input.UpdatePosition(device, pos), "");
  PORT_EXPECT_DEBUG_DEATH(input.UpdateRotation(device, rot), "");

  profile.rotation_dof = DeviceProfile::kRealDof;
  profile.position_dof = DeviceProfile::kRealDof;

  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  input.AdvanceFrame(kDeltaTime);

  // Checking to make sure the above UpdatePosition and UpdateRotation didn't
  // write anything
  EXPECT_NEAR(input.GetDofPosition(device)[0], 0, kEpsilon);
  EXPECT_NEAR(input.GetDofDelta(device)[0], 0, kEpsilon);
  EXPECT_NEAR(input.GetDofRotation(device)[1], 0, kEpsilon);
  EXPECT_NEAR(input.GetDofAngularDelta(device)[1], 0, kEpsilon);
  EXPECT_NEAR(input.GetDofWorldFromObjectMatrix(device)(1, 2), 0, kEpsilon);
  EXPECT_NEAR(input.GetDofWorldFromObjectMatrix(device)(0, 3), 0, kEpsilon);

  input.UpdatePosition(device, pos);
  input.UpdateRotation(device, half_rot);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_NEAR(input.GetDofPosition(device)[0], pos[0], kEpsilon);
  EXPECT_NEAR(input.GetDofDelta(device)[0], pos[0], kEpsilon);
  EXPECT_NEAR(input.GetDofRotation(device)[1], half_rot[1], kEpsilon);
  EXPECT_NEAR(input.GetDofAngularDelta(device)[1], half_rot[1], kEpsilon);
  EXPECT_NEAR(input.GetDofWorldFromObjectMatrix(device)(1, 2), -1.f, kEpsilon);
  EXPECT_NEAR(input.GetDofWorldFromObjectMatrix(device)(0, 3), pos[0],
              kEpsilon);

  input.UpdatePosition(device, -1.f * pos);
  input.UpdateRotation(device, rot);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_NEAR(input.GetDofPosition(device)[0], -1.f * pos[0], kEpsilon);
  EXPECT_NEAR(input.GetDofDelta(device)[0], -2.f * pos[0], kEpsilon);
  EXPECT_NEAR(input.GetDofRotation(device)[1], rot[1], kEpsilon);
  EXPECT_NEAR(input.GetDofAngularDelta(device)[1], half_rot[1], kEpsilon);
  EXPECT_NEAR(input.GetDofWorldFromObjectMatrix(device)(2, 2), -1.f, kEpsilon);
  EXPECT_NEAR(input.GetDofWorldFromObjectMatrix(device)(0, 3), -1.f * pos[0],
              kEpsilon);

  input.DisconnectDevice(device);
  EXPECT_TRUE(!input.IsConnected(device));
}

TEST(InputManagerDeathTest, Eye) {
  InputManager input;
  const auto device = InputManager::kHmd;
  const size_t left_eye = 0;
  const size_t right_eye = 1;
  const size_t num_eyes = 2;
  const mathfu::mat4 left_eye_from_head =
      mathfu::mat4::FromTranslationVector(mathfu::vec3(1, 0, 0));
  const mathfu::mat4 right_eye_from_head =
      mathfu::mat4::FromTranslationVector(mathfu::vec3(-1, 0, 0));
  const mathfu::mat4 left_screen_from_eye =
      mathfu::mat4::FromTranslationVector(mathfu::vec3(2, 0, 0));
  const mathfu::mat4 right_screen_from_eye =
      mathfu::mat4::FromTranslationVector(mathfu::vec3(-2, 0, 0));
  const mathfu::rectf left_fov(1, 0, 0, 0);
  const mathfu::rectf right_fov(0, 1, 0, 0);

  PORT_EXPECT_DEBUG_DEATH(input.GetEyeFromHead(device, left_eye), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetScreenFromEye(device, left_eye), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetEyeFOV(device, left_eye), "");

  PORT_EXPECT_DEBUG_DEATH(input.UpdateEye(device, left_eye, left_eye_from_head,
                                          left_screen_from_eye, left_fov),
                          "");

  DeviceProfile profile;
  profile.eyes.resize(2);
  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  input.AdvanceFrame(kDeltaTime);

  // Checking to make sure the above UpdateEye didn't write anything
  EXPECT_NEAR(input.GetEyeFromHead(device, left_eye)(0, 3), 0, 0.00001f);
  EXPECT_NEAR(input.GetScreenFromEye(device, left_eye)(0, 3), 0, 0.00001f);

  PORT_EXPECT_DEBUG_DEATH(input.GetEyeFromHead(device, num_eyes), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetScreenFromEye(device, num_eyes), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetEyeFOV(device, num_eyes), "");
  PORT_EXPECT_DEBUG_DEATH(
      input.UpdateEye(device, num_eyes, left_eye_from_head, left_screen_from_eye, left_fov), "");

  input.UpdateEye(device, left_eye, left_eye_from_head, left_screen_from_eye,
                  left_fov);
  input.UpdateEye(device, right_eye, right_eye_from_head, right_screen_from_eye,
                  right_fov);

  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetEyeFromHead(device, left_eye)(0, 3), 1);
  EXPECT_EQ(input.GetEyeFromHead(device, right_eye)(0, 3), -1);
  EXPECT_EQ(input.GetScreenFromEye(device, left_eye)(0, 3), 2);
  EXPECT_EQ(input.GetScreenFromEye(device, right_eye)(0, 3), -2);
  EXPECT_EQ(input.GetEyeFOV(device, left_eye).pos.x, left_fov.pos.x);
  EXPECT_EQ(input.GetEyeFOV(device, left_eye).pos.y, left_fov.pos.y);
  EXPECT_EQ(input.GetEyeFOV(device, right_eye).pos.x, right_fov.pos.x);
  EXPECT_EQ(input.GetEyeFOV(device, right_eye).pos.y, right_fov.pos.y);

  input.DisconnectDevice(device);
  EXPECT_TRUE(!input.IsConnected(device));
}

TEST(InputManagerDeathTest, Joystick) {
  InputManager input;
  const auto device = InputManager::kController;
  const auto joystick = InputManager::kLeftJoystick;
  const auto invalid_joystick = InputManager::kRightJoystick;

  PORT_EXPECT_DEBUG_DEATH(input.GetJoystickValue(device, joystick), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetJoystickDelta(device, joystick), "");

  PORT_EXPECT_DEBUG_DEATH(
      input.UpdateJoystick(device, joystick, mathfu::kOnes2f), "");

  DeviceProfile profile;
  profile.joysticks.resize(1);
  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  input.AdvanceFrame(kDeltaTime);

  // Checking to make sure the above UpdateEye didn't write anything
  EXPECT_NEAR(input.GetJoystickValue(device, joystick)[0], 0, 0.00001f);

  PORT_EXPECT_DEBUG_DEATH(input.GetJoystickValue(device, invalid_joystick), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetJoystickDelta(device, invalid_joystick), "");
  PORT_EXPECT_DEBUG_DEATH(
      input.UpdateJoystick(device, invalid_joystick, mathfu::kOnes2f), "");

  input.UpdateJoystick(device, joystick, mathfu::kOnes2f);

  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetJoystickValue(device, joystick)[0], 1);
  EXPECT_EQ(input.GetJoystickDelta(device, joystick)[0], 1);

  input.UpdateJoystick(device, joystick, -1.0f * mathfu::kOnes2f);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetJoystickValue(device, joystick)[0], -1);
  EXPECT_EQ(input.GetJoystickDelta(device, joystick)[0], -2);

  // Check that we are clamping values at (-1.0, 1.0).
  input.UpdateJoystick(device, joystick, mathfu::vec2(-1.001f, 0));
  input.AdvanceFrame(kDeltaTime);
  EXPECT_THAT(input.GetJoystickValue(device, joystick),
              NearMathfu(mathfu::vec2(-1.0f, 0.0f), kEpsilon));

  input.UpdateJoystick(device, joystick, mathfu::vec2(1.001f, 0));
  input.AdvanceFrame(kDeltaTime);
  EXPECT_THAT(input.GetJoystickValue(device, joystick),
              NearMathfu(mathfu::vec2(1.0f, 0.0f), kEpsilon));

  input.UpdateJoystick(device, joystick, mathfu::vec2(0, -1.001f));
  input.AdvanceFrame(kDeltaTime);
  EXPECT_THAT(input.GetJoystickValue(device, joystick),
              NearMathfu(mathfu::vec2(0.0f, -1.0f), kEpsilon));

  input.UpdateJoystick(device, joystick, mathfu::vec2(0, 1.001f));
  input.AdvanceFrame(kDeltaTime);
  EXPECT_THAT(input.GetJoystickValue(device, joystick),
              NearMathfu(mathfu::vec2(0.0f, 1.0f), kEpsilon));

  input.DisconnectDevice(device);
  EXPECT_TRUE(!input.IsConnected(device));
}

TEST(InputManagerDeathTest, Touch) {
  InputManager input;
  InputManager::TouchpadId pad = InputManager::kPrimaryTouchpadId;
  const auto device = InputManager::kController;
  const float kInvalidTouchLocation = -1;

  PORT_EXPECT_DEBUG_DEATH(input.GetTouchLocation(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchDelta(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.IsValidTouch(device), "");

  PORT_EXPECT_DEBUG_DEATH(
      input.UpdateTouch(device, pad, 0, mathfu::kOnes2f, true), "");

  DeviceProfile profile;
  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  PORT_EXPECT_DEBUG_DEATH(input.IsValidTouch(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchLocation(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchDelta(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchVelocity(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchGestureDirection(device), "");

  input.DisconnectDevice(device);
  EXPECT_TRUE(!input.IsConnected(device));

  profile.touchpads.resize(1);
  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  input.AdvanceFrame(kDeltaTime);

  // Checking to make sure the above UpdateTouch didn't write anything
  EXPECT_EQ(input.GetTouchLocation(device)[0], kInvalidTouchLocation);
  EXPECT_EQ(input.GetTouchDelta(device)[0], 0);
  EXPECT_TRUE(!input.IsValidTouch(device));
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kJustPressed, 0);
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kPressed, 0);
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kJustReleased, 0);
  EXPECT_NE(input.GetTouchState(device) & InputManager::kReleased, 0);

  input.UpdateTouch(device, pad, 0, mathfu::kOnes2f, true);

  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouchLocation(device)[0], 1);
  EXPECT_EQ(input.GetTouchDelta(device)[0], 0);  // Delta == 0 on first frame.
  EXPECT_EQ(input.GetTouchVelocity(device)[0], 0);  // Same for velocity.
  EXPECT_EQ(input.GetTouchGestureDirection(device),
            InputManager::GestureDirection::kNone);
  EXPECT_TRUE(input.IsValidTouch(device));
  EXPECT_NE(input.GetTouchState(device) & InputManager::kJustPressed, 0);
  EXPECT_NE(input.GetTouchState(device) & InputManager::kPressed, 0);
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kJustReleased, 0);
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kReleased, 0);

  input.UpdateTouch(device, pad, 0, mathfu::kZeros2f, true);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouchLocation(device)[0], 0);
  EXPECT_EQ(input.GetTouchDelta(device)[0], -1);
  EXPECT_LT(input.GetTouchVelocity(device)[0], 0);
  EXPECT_EQ(input.GetTouchGestureDirection(device),
            InputManager::GestureDirection::kNone);
  EXPECT_TRUE(input.IsValidTouch(device));
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kJustPressed, 0);
  EXPECT_NE(input.GetTouchState(device) & InputManager::kPressed, 0);
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kJustReleased, 0);
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kReleased, 0);

  input.UpdateTouch(device, pad, 0, mathfu::kOnes2f, false);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouchLocation(device)[0], kInvalidTouchLocation);
  EXPECT_NEAR(input.GetTouchDelta(device)[0], 0, 0.00001f);
  EXPECT_TRUE(!input.IsValidTouch(device));
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kJustPressed, 0);
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kPressed, 0);
  EXPECT_NE(input.GetTouchState(device) & InputManager::kJustReleased, 0);
  EXPECT_NE(input.GetTouchState(device) & InputManager::kReleased, 0);

  input.UpdateTouch(device, pad, 0, mathfu::kOnes2f, false);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouchLocation(device)[0], kInvalidTouchLocation);
  EXPECT_NEAR(input.GetTouchDelta(device)[0], 0, 0.00001f);
  EXPECT_TRUE(!input.IsValidTouch(device));
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kJustPressed, 0);
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kPressed, 0);
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kJustReleased, 0);
  EXPECT_NE(input.GetTouchState(device) & InputManager::kReleased, 0);

  // Check that we are clamping values at (-1.0, 1.0).
  input.UpdateTouch(device, pad, 0, mathfu::vec2(-0.001f, 0), true);
  input.AdvanceFrame(kDeltaTime);
  EXPECT_THAT(input.GetTouchLocation(device),
              NearMathfu(mathfu::vec2(0.0f, 0.0f), kEpsilon));

  input.UpdateTouch(device, pad, 0, mathfu::vec2(1.001f, 0), true);
  input.AdvanceFrame(kDeltaTime);
  EXPECT_THAT(input.GetTouchLocation(device),
              NearMathfu(mathfu::vec2(1.0f, 0.0f), kEpsilon));

  input.UpdateTouch(device, pad, 0, mathfu::vec2(0, -0.001f), true);
  input.AdvanceFrame(kDeltaTime);
  EXPECT_THAT(input.GetTouchLocation(device),
              NearMathfu(mathfu::vec2(0.0f, 0.0f), kEpsilon));

  input.UpdateTouch(device, pad, 0, mathfu::vec2(0, 1.001f), true);
  input.AdvanceFrame(kDeltaTime);
  EXPECT_THAT(input.GetTouchLocation(device),
              NearMathfu(mathfu::vec2(0.0f, 1.0f), kEpsilon));

  input.DisconnectDevice(device);
  EXPECT_TRUE(!input.IsConnected(device));
}

TEST(InputManagerDeathTest, Touchpads) {
  InputManager input;
  InputManager::TouchpadId pad1 = 0;
  InputManager::TouchpadId pad2 = 1;
  const auto device = InputManager::kController;
  const float kInvalidTouchLocation = -1;

  PORT_EXPECT_DEBUG_DEATH(input.GetTouchLocation(device, pad2), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchDelta(device, pad2), "");
  PORT_EXPECT_DEBUG_DEATH(input.IsValidTouch(device, pad2), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchLocation(device, pad1), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchDelta(device, pad1), "");
  PORT_EXPECT_DEBUG_DEATH(input.IsValidTouch(device, pad1), "");

  PORT_EXPECT_DEBUG_DEATH(
      input.UpdateTouch(device, pad1, 0, mathfu::kOnes2f, true), "");
  PORT_EXPECT_DEBUG_DEATH(
      input.UpdateTouch(device, pad2, 0, mathfu::kOnes2f, true), "");

  DeviceProfile profile;
  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  PORT_EXPECT_DEBUG_DEATH(input.IsValidTouch(device, pad1), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchLocation(device, pad1), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchDelta(device, pad1), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchVelocity(device, pad1), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchGestureDirection(device, pad1), "");
  PORT_EXPECT_DEBUG_DEATH(input.IsValidTouch(device, pad2), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchLocation(device, pad2), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchDelta(device, pad2), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchVelocity(device, pad2), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetTouchGestureDirection(device, pad2), "");

  input.DisconnectDevice(device);
  EXPECT_TRUE(!input.IsConnected(device));

  profile.touchpads.resize(2);
  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  input.AdvanceFrame(kDeltaTime);

  // Checking to make sure the above UpdateTouch didn't write anything
  EXPECT_EQ(input.GetTouchLocation(device, pad1)[0], kInvalidTouchLocation);
  EXPECT_EQ(input.GetTouchDelta(device, pad1)[0], 0);
  EXPECT_TRUE(!input.IsValidTouch(device, pad1));
  EXPECT_EQ(input.GetTouchState(device, pad1) & InputManager::kJustPressed, 0);
  EXPECT_EQ(input.GetTouchState(device, pad1) & InputManager::kPressed, 0);
  EXPECT_EQ(input.GetTouchState(device, pad1) & InputManager::kJustReleased, 0);
  EXPECT_NE(input.GetTouchState(device, pad1) & InputManager::kReleased, 0);
  EXPECT_EQ(input.GetTouchLocation(device, pad2)[0], kInvalidTouchLocation);
  EXPECT_EQ(input.GetTouchDelta(device, pad2)[0], 0);
  EXPECT_TRUE(!input.IsValidTouch(device, pad2));
  EXPECT_EQ(input.GetTouchState(device, pad2) & InputManager::kJustPressed, 0);
  EXPECT_EQ(input.GetTouchState(device, pad2) & InputManager::kPressed, 0);
  EXPECT_EQ(input.GetTouchState(device, pad2) & InputManager::kJustReleased, 0);
  EXPECT_NE(input.GetTouchState(device, pad2) & InputManager::kReleased, 0);

  input.UpdateTouch(device, pad1, 0, mathfu::kOnes2f, true);
  input.UpdateTouch(device, pad2, 0, mathfu::kZeros2f, false);

  input.AdvanceFrame(kDeltaTime);

  // Touch on pad 1 was handled correctly
  EXPECT_EQ(input.GetTouchLocation(device)[0], 1);
  EXPECT_EQ(input.GetTouchDelta(device)[0], 0);  // Delta == 0 on first frame.
  EXPECT_EQ(input.GetTouchVelocity(device)[0], 0);  // Same for velocity.
  EXPECT_EQ(input.GetTouchGestureDirection(device),
            InputManager::GestureDirection::kNone);
  EXPECT_TRUE(input.IsValidTouch(device));
  EXPECT_NE(input.GetTouchState(device) & InputManager::kJustPressed, 0);
  EXPECT_NE(input.GetTouchState(device) & InputManager::kPressed, 0);
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kJustReleased, 0);
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kReleased, 0);

  // Explicitly asking for pad 1 touch 0 should also work
  EXPECT_EQ(input.GetTouchLocation(device, pad1, 0)[0], 1);
  EXPECT_EQ(input.GetTouchDelta(device, pad1, 0)[0],
            0);  // Delta == 0 on first frame.
  EXPECT_EQ(input.GetTouchVelocity(device, pad1, 0)[0],
            0);  // Same for velocity.
  EXPECT_EQ(input.GetTouchGestureDirection(device, pad1),
            InputManager::GestureDirection::kNone);
  EXPECT_TRUE(input.IsValidTouch(device, pad1, 0));
  EXPECT_NE(input.GetTouchState(device, pad1, 0) & InputManager::kJustPressed,
            0);
  EXPECT_NE(input.GetTouchState(device, pad1, 0) & InputManager::kPressed, 0);
  EXPECT_EQ(input.GetTouchState(device, pad1, 0) & InputManager::kJustReleased,
            0);
  EXPECT_EQ(input.GetTouchState(device, pad1, 0) & InputManager::kReleased, 0);

  // pad 2 should have no touch
  EXPECT_EQ(input.GetTouchLocation(device, pad2)[0], kInvalidTouchLocation);
  EXPECT_EQ(input.GetTouchDelta(device, pad2)[0], 0);
  EXPECT_TRUE(!input.IsValidTouch(device, pad2));
  EXPECT_EQ(input.GetTouchState(device, pad2) & InputManager::kJustPressed, 0);
  EXPECT_EQ(input.GetTouchState(device, pad2) & InputManager::kPressed, 0);
  EXPECT_EQ(input.GetTouchState(device, pad2) & InputManager::kJustReleased, 0);
  EXPECT_NE(input.GetTouchState(device, pad2) & InputManager::kReleased, 0);

  // Touching both touchpads should show correct deltas for both.
  input.UpdateTouch(device, pad1, 0, mathfu::kZeros2f, true);
  input.UpdateTouch(device, pad2, 0, mathfu::kOnes2f / 2.0f, true);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouchLocation(device)[0], 0);
  EXPECT_EQ(input.GetTouchDelta(device)[0], -1);
  EXPECT_LT(input.GetTouchVelocity(device)[0], 0);
  EXPECT_EQ(input.GetTouchGestureDirection(device),
            InputManager::GestureDirection::kNone);
  EXPECT_TRUE(input.IsValidTouch(device));
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kJustPressed, 0);
  EXPECT_NE(input.GetTouchState(device) & InputManager::kPressed, 0);
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kJustReleased, 0);
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kReleased, 0);

  EXPECT_EQ(input.GetTouchLocation(device, pad2)[0], 0.5f);
  EXPECT_EQ(input.GetTouchDelta(device, pad2)[0], 0);
  EXPECT_EQ(input.GetTouchVelocity(device, pad2)[0], 0);
  EXPECT_EQ(input.GetTouchGestureDirection(device, pad2),
            InputManager::GestureDirection::kNone);
  EXPECT_TRUE(input.IsValidTouch(device, pad2));
  EXPECT_NE(input.GetTouchState(device, pad2) & InputManager::kJustPressed, 0);
  EXPECT_NE(input.GetTouchState(device, pad2) & InputManager::kPressed, 0);
  EXPECT_EQ(input.GetTouchState(device, pad2) & InputManager::kJustReleased, 0);
  EXPECT_EQ(input.GetTouchState(device, pad2) & InputManager::kReleased, 0);

  input.UpdateTouch(device, pad1, 0, mathfu::kZeros2f, false);
  input.UpdateTouch(device, pad2, 0, mathfu::kOnes2f, true);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouchLocation(device)[0], -1);
  EXPECT_EQ(input.GetTouchDelta(device)[0], 0);
  EXPECT_LT(input.GetTouchVelocity(device)[0], -1);
  EXPECT_TRUE(!input.IsValidTouch(device));
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kJustPressed, 0);
  EXPECT_EQ(input.GetTouchState(device) & InputManager::kPressed, 0);
  EXPECT_NE(input.GetTouchState(device) & InputManager::kJustReleased, 0);
  EXPECT_NE(input.GetTouchState(device) & InputManager::kReleased, 0);

  EXPECT_EQ(input.GetTouchLocation(device, pad2)[0], 1);
  EXPECT_EQ(input.GetTouchDelta(device, pad2)[0], 0.5f);
  EXPECT_GT(input.GetTouchVelocity(device, pad2)[0], 1);
  EXPECT_EQ(input.GetTouchGestureDirection(device, pad2),
            InputManager::GestureDirection::kNone);
  EXPECT_TRUE(input.IsValidTouch(device, pad2));
  EXPECT_EQ(input.GetTouchState(device, pad2) & InputManager::kJustPressed, 0);
  EXPECT_NE(input.GetTouchState(device, pad2) & InputManager::kPressed, 0);
  EXPECT_EQ(input.GetTouchState(device, pad2) & InputManager::kJustReleased, 0);
  EXPECT_EQ(input.GetTouchState(device, pad2) & InputManager::kReleased, 0);

  input.DisconnectDevice(device);
  EXPECT_TRUE(!input.IsConnected(device));
}

TEST(InputManagerDeathTest, Multitouch) {
  InputManager input;
  const InputManager::TouchpadId pad = 0;
  const InputManager::TouchId touch1 = 0;
  const InputManager::TouchId touch2 = 34512;
  const InputManager::TouchId touch3 = 1;

  const auto device = InputManager::kController;

  PORT_EXPECT_DEBUG_DEATH(input.GetTouches(device, pad), "");

  DeviceProfile profile;
  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  PORT_EXPECT_DEBUG_DEATH(input.GetTouches(device, pad), "");

  input.DisconnectDevice(device);
  EXPECT_TRUE(!input.IsConnected(device));

  profile.touchpads.resize(1);
  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  input.AdvanceFrame(kDeltaTime);

  // Updating a touch that has never actually been pressed shouldn't do
  // anything.
  input.UpdateTouch(device, pad, touch1, mathfu::kOnes2f, false);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouches(device, pad).size(), 0);

  // A single valid touch
  input.UpdateTouch(device, pad, touch1, mathfu::kOnes2f, true);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouches(device, pad).size(), 1);
  EXPECT_EQ(input.GetTouches(device, pad)[0], touch1);

  input.UpdateTouch(device, pad, touch1, mathfu::kOnes2f * 0.5f, true);
  input.UpdateTouch(device, pad, touch2, mathfu::kZeros2f, true);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouches(device, pad).size(), 2);
  EXPECT_EQ(input.GetTouches(device, pad)[0], touch1);
  EXPECT_EQ(input.GetTouches(device, pad)[1], touch2);
  EXPECT_EQ(input.GetTouchLocation(device, pad, touch1)[0], 0.5f);
  EXPECT_EQ(input.GetTouchLocation(device, pad, touch2)[0], 0);

  input.UpdateTouch(device, pad, touch1, mathfu::kOnes2f * 0.5f, false);
  input.UpdateTouch(device, pad, touch2, mathfu::kZeros2f, true);
  input.UpdateTouch(device, pad, touch3, mathfu::kOnes2f, true);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouches(device, pad).size(), 2);
  EXPECT_EQ(input.GetTouches(device, pad)[0], touch2);
  EXPECT_EQ(input.GetTouches(device, pad)[1], touch3);
  EXPECT_EQ(input.GetTouchLocation(device, pad, touch2)[0], 0);
  EXPECT_EQ(input.GetTouchLocation(device, pad, touch3)[0], 1);

  // Check that there's still data for the released touch1:
  EXPECT_LT(input.GetTouchVelocity(device, pad, touch1)[0], -1);
  auto touch_state = input.GetTouchState(device, pad, touch1);
  EXPECT_EQ(touch_state & InputManager::kJustPressed, 0);
  EXPECT_EQ(touch_state & InputManager::kPressed, 0);
  EXPECT_NE(touch_state & InputManager::kJustReleased, 0);
  EXPECT_NE(touch_state & InputManager::kReleased, 0);

  input.UpdateTouch(device, pad, touch2, mathfu::kZeros2f, false);
  input.UpdateTouch(device, pad, touch3, mathfu::kOnes2f, false);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouches(device, pad).size(), 0);

  // Released for more than 1 frame, so data should be cleaned up:
  EXPECT_EQ(input.GetTouchVelocity(device, pad, touch1)[0], 0);

  input.DisconnectDevice(device);
  EXPECT_TRUE(!input.IsConnected(device));
}

TEST(InputManager, TouchGesture_Implicit) {
  InputManager input;
  const auto device = InputManager::kController;

  DeviceProfile profile;
  profile.touchpads.resize(1);
  input.ConnectDevice(device, profile);
  EXPECT_EQ(input.IsTouchGestureAvailable(device), false);

  input.AdvanceFrame(kDeltaTime);

  // Test left fling.
  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::vec2(1, 0), true);
  input.AdvanceFrame(kDeltaTime);

  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::vec2(0, 0), true);
  input.AdvanceFrame(kDeltaTime);

  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::kZeros2f, false);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouchGestureDirection(device),
            InputManager::GestureDirection::kLeft);

  // Test right fling.
  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::vec2(0, 0), true);
  input.AdvanceFrame(kDeltaTime);

  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::vec2(1, 0), true);
  input.AdvanceFrame(kDeltaTime);

  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::kZeros2f, false);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouchGestureDirection(device),
            InputManager::GestureDirection::kRight);

  // Test up fling.
  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::vec2(0, 1), true);
  input.AdvanceFrame(kDeltaTime);

  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::vec2(0, 0), true);
  input.AdvanceFrame(kDeltaTime);

  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::kZeros2f, false);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouchGestureDirection(device),
            InputManager::GestureDirection::kUp);

  // Test down fling.
  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::vec2(0, 0), true);
  input.AdvanceFrame(kDeltaTime);

  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::vec2(0, 1), true);
  input.AdvanceFrame(kDeltaTime);

  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::kZeros2f, false);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouchGestureDirection(device),
            InputManager::GestureDirection::kDown);
}

TEST(InputManager, TouchGesture_Explicit) {
  InputManager input;
  const auto device = InputManager::kController;

  DeviceProfile profile;
  profile.touchpads.resize(1);
  profile.touchpads[0].has_gestures = true;
  input.ConnectDevice(device, profile);

  input.AdvanceFrame(kDeltaTime);

  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::vec2(0, 0), true);
  input.UpdateGesture(device, InputManager::kPrimaryTouchpadId,
                      InputManager::GestureType::kNone,
                      InputManager::GestureDirection::kNone, mathfu::vec2(0, 0),
                      mathfu::vec2(0, 0));
  input.AdvanceFrame(kDeltaTime);

  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::vec2(0, 0), true);
  input.UpdateGesture(device, InputManager::kPrimaryTouchpadId,
                      InputManager::GestureType::kScrollStart,
                      InputManager::GestureDirection::kNone, mathfu::vec2(0, 0),
                      mathfu::vec2(0, 0));
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouchDelta(device).x, 0.f);
  EXPECT_EQ(input.GetTouchDelta(device).y, 0.f);
  EXPECT_EQ(input.GetTouchVelocity(device).x, 0.f);
  EXPECT_EQ(input.GetTouchVelocity(device).y, 0.f);
  EXPECT_EQ(input.GetTouchGestureDirection(device),
            InputManager::GestureDirection::kNone);
  EXPECT_EQ(input.GetTouchGestureType(device),
            InputManager::GestureType::kScrollStart);

  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::vec2(0, 0), true);
  input.UpdateGesture(device, InputManager::kPrimaryTouchpadId,
                      InputManager::GestureType::kFling,
                      InputManager::GestureDirection::kUp, mathfu::vec2(1, 2),
                      mathfu::vec2(3, 4));
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetTouchDelta(device).x, 1.f);
  EXPECT_EQ(input.GetTouchDelta(device).y, 2.f);
  EXPECT_EQ(input.GetTouchVelocity(device).x, 3.f);
  EXPECT_EQ(input.GetTouchVelocity(device).y, 4.f);
  EXPECT_EQ(input.GetTouchGestureDirection(device),
            InputManager::GestureDirection::kUp);
  EXPECT_EQ(input.GetTouchGestureType(device),
            InputManager::GestureType::kFling);
}

TEST(InputManager, TouchGesture_InitialDirection) {
  InputManager input;
  const auto device = InputManager::kController;

  DeviceProfile profile;
  profile.touchpads.resize(1);
  profile.touchpads[0].has_gestures = true;
  input.ConnectDevice(device, profile);
  EXPECT_EQ(input.IsTouchGestureAvailable(device), true);

  input.AdvanceFrame(kDeltaTime);

  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::vec2(0, 0), true);
  input.UpdateGesture(device, InputManager::kPrimaryTouchpadId,
                      InputManager::GestureType::kNone,
                      InputManager::GestureDirection::kNone, mathfu::vec2(0, 0),
                      mathfu::vec2(1, 3));
  input.AdvanceFrame(kDeltaTime);

  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::vec2(0, 0), true);
  input.UpdateGesture(device, InputManager::kPrimaryTouchpadId,
                      InputManager::GestureType::kScrollStart,
                      InputManager::GestureDirection::kNone,
                      mathfu::vec2(0.3f, 0.5f), mathfu::vec2(0, 0));
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetInitialDisplacementAxis(device).x, 0.f);
  EXPECT_EQ(input.GetInitialDisplacementAxis(device).y, 1.f);
  EXPECT_EQ(input.GetLockedTouchDelta(device).x, 0.f);
  EXPECT_EQ(input.GetLockedTouchDelta(device).y, 0.5f);
  EXPECT_EQ(input.GetTouchGestureDirection(device),
            InputManager::GestureDirection::kNone);

  input.UpdateTouch(device, InputManager::kPrimaryTouchpadId, 0,
                    mathfu::vec2(0, 0), true);
  input.UpdateGesture(device, InputManager::kPrimaryTouchpadId,
                      InputManager::GestureType::kFling,
                      InputManager::GestureDirection::kUp, mathfu::vec2(1, 2),
                      mathfu::vec2(3, 4));
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetInitialDisplacementAxis(device).x, 0.f);
  EXPECT_EQ(input.GetInitialDisplacementAxis(device).y, 0.f);
  EXPECT_EQ(input.GetLockedTouchDelta(device).x, 0.f);
  EXPECT_EQ(input.GetLockedTouchDelta(device).y, 0.f);
  EXPECT_EQ(input.GetTouchGestureDirection(device),
            InputManager::GestureDirection::kUp);
}

TEST(InputManagerDeathTest, Scroll) {
  InputManager input;
  const auto device = InputManager::kController;

  PORT_EXPECT_DEBUG_DEATH(input.GetScrollDelta(device), "");

  DeviceProfile profile;
  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  PORT_EXPECT_DEBUG_DEATH(input.GetScrollDelta(device), "");
  input.DisconnectDevice(device);

  PORT_EXPECT_DEBUG_DEATH(input.UpdateScroll(device, 10), "");

  profile.scroll_wheels.resize(1);
  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  input.AdvanceFrame(kDeltaTime);

  // Checking to make sure the above UpdateScroll didn't write anything
  EXPECT_NEAR(input.GetScrollDelta(device), 0, 0.00001f);

  input.UpdateScroll(device, 1);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetScrollDelta(device), 1);

  input.UpdateScroll(device, -1);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetScrollDelta(device), -1);

  input.DisconnectDevice(device);
  EXPECT_TRUE(!input.IsConnected(device));
}

TEST(InputManagerDeathTest, Battery) {
  InputManager input;
  const auto device = InputManager::kController;

  PORT_EXPECT_DEBUG_DEATH(input.GetScrollDelta(device), "");

  DeviceProfile profile;
  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  PORT_EXPECT_DEBUG_DEATH(input.GetBatteryCharge(device), "");
  PORT_EXPECT_DEBUG_DEATH(input.GetBatteryState(device), "");
  input.DisconnectDevice(device);

  PORT_EXPECT_DEBUG_DEATH(
      input.UpdateBattery(device, InputManager::BatteryState::kUnknown, 0), "");

  profile.battery.emplace();
  input.ConnectDevice(device, profile);
  EXPECT_TRUE(input.IsConnected(device));

  input.AdvanceFrame(kDeltaTime);

  // Checking to make sure the above UpdateBattery didn't write anything
  EXPECT_EQ(input.GetBatteryCharge(device),
            InputManager::kInvalidBatteryCharge);
  EXPECT_EQ(input.GetBatteryState(device),
            InputManager::BatteryState::kUnknown);

  input.UpdateBattery(device, InputManager::BatteryState::kDischarging, 50);
  input.AdvanceFrame(kDeltaTime);

  EXPECT_EQ(input.GetBatteryCharge(device), 50);
  EXPECT_EQ(input.GetBatteryState(device),
            InputManager::BatteryState::kDischarging);

  input.DisconnectDevice(device);
  EXPECT_TRUE(!input.IsConnected(device));
}
}  // namespace
}  // namespace lull
