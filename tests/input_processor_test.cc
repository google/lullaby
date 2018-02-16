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

#include "lullaby/modules/input_processor/input_processor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/events/input_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/dispatcher/queued_dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/generated/transform_def_generated.h"

namespace lull {
namespace {

static const float kEpsilon = 0.0001f;
const Clock::duration kDeltaTime = std::chrono::milliseconds(17);
const Clock::duration kLongPressTime = std::chrono::milliseconds(500);

// A macro to expand an event list into the contents of an enum.
#define LULLABY_GENERATE_ENUM(Enum, Name) Enum,
// clang-format off
/// Events that are sent on a per-device basis:
enum DeviceEventType {
  LULLABY_DEVICE_EVENT_LIST(LULLABY_GENERATE_ENUM)
  kNumDeviceEventTypes
};

// Events that are sent on a per-button and device basis:
enum ButtonEventType {
  LULLABY_BUTTON_EVENT_LIST(LULLABY_GENERATE_ENUM)
  kNumButtonEventTypes
};
// clang-format on
#undef LULLABY_GENERATE_ENUM

class InputProcessorTest : public testing::Test {
 public:
  void SetUp() override {
    registry_.reset(new Registry());

    dispatcher_ = registry_->Create<Dispatcher>();
    input_manager_ = registry_->Create<InputManager>();
    input_processor_ = registry_->Create<InputProcessor>(registry_.get());
    entity_factory_ = registry_->Create<EntityFactory>(registry_.get());
    transform_system_ = entity_factory_->CreateSystem<TransformSystem>();
    dispatcher_system_ = entity_factory_->CreateSystem<DispatcherSystem>();
    entity_factory_->Initialize();

    InputManager::DeviceParams params;
    params.num_buttons = 2;
    input_manager_->ConnectDevice(InputManager::kController, params);
  }

 protected:
  std::unique_ptr<Registry> registry_;
  Dispatcher* dispatcher_;
  EntityFactory* entity_factory_;
  TransformSystem* transform_system_;
  DispatcherSystem* dispatcher_system_;
  InputManager* input_manager_;
  InputProcessor* input_processor_;
};

class InputEventListener {
 public:
  struct ButtonEventCall {
    InputManager::DeviceType device = InputManager::kMaxNumDeviceTypes;
    InputManager::ButtonId button = InputManager::kInvalidButton;

    void Reset() {
      device = InputManager::kMaxNumDeviceTypes;
      button = InputManager::kInvalidButton;
    }
  };

  InputManager::DeviceType device_event_calls_[kNumDeviceEventTypes];
  ButtonEventCall button_event_calls_[kNumButtonEventTypes];

  InputEventListener(Registry* registry, Entity target, string_view prefix) {
    registry_ = registry;
    target_ = target;
    Reset();
    if (target == kNullEntity) {
      auto* dispatcher = registry_->Get<Dispatcher>();

      dispatcher->Connect(
          Hash(prefix.to_string() + "FocusStartEvent"), this,
          [this](const EventWrapper& event) {
            device_event_calls_[kFocusStart] = event.GetValueWithDefault(
                kDeviceHash, InputManager::kMaxNumDeviceTypes);
          });
      dispatcher->Connect(
          Hash(prefix.to_string() + "FocusStopEvent"), this,
          [this](const EventWrapper& event) {
            device_event_calls_[kFocusStop] = event.GetValueWithDefault(
                kDeviceHash, InputManager::kMaxNumDeviceTypes);
          });
      dispatcher->Connect(
          Hash(prefix.to_string() + "PressEvent"), this,
          [this](const EventWrapper& event) {
            button_event_calls_[kPress].device = event.GetValueWithDefault(
                kDeviceHash, InputManager::kMaxNumDeviceTypes);
            button_event_calls_[kPress].button = event.GetValueWithDefault(
                kButtonHash, InputManager::kInvalidButton);
          });
      dispatcher->Connect(
          Hash(prefix.to_string() + "ReleaseEvent"), this,
          [this](const EventWrapper& event) {
            button_event_calls_[kRelease].device = event.GetValueWithDefault(
                kDeviceHash, InputManager::kMaxNumDeviceTypes);
            button_event_calls_[kRelease].button = event.GetValueWithDefault(
                kButtonHash, InputManager::kInvalidButton);
          });
      dispatcher->Connect(
          Hash(prefix.to_string() + "ClickEvent"), this,
          [this](const EventWrapper& event) {
            button_event_calls_[kClick].device = event.GetValueWithDefault(
                kDeviceHash, InputManager::kMaxNumDeviceTypes);
            button_event_calls_[kClick].button = event.GetValueWithDefault(
                kButtonHash, InputManager::kInvalidButton);
          });
      dispatcher->Connect(
          Hash(prefix.to_string() + "LongPressEvent"), this,
          [this](const EventWrapper& event) {
            button_event_calls_[kLongPress].device = event.GetValueWithDefault(
                kDeviceHash, InputManager::kMaxNumDeviceTypes);
            button_event_calls_[kLongPress].button = event.GetValueWithDefault(
                kButtonHash, InputManager::kInvalidButton);
          });
    } else {
      auto* dispatcher_system = registry_->Get<DispatcherSystem>();

      dispatcher_system->Connect(
          target, Hash(prefix.to_string() + "FocusStartEvent"), this,
          [this](const EventWrapper& event) {
            device_event_calls_[kFocusStart] = event.GetValueWithDefault(
                kDeviceHash, InputManager::kMaxNumDeviceTypes);
          });
      dispatcher_system->Connect(
          target, Hash(prefix.to_string() + "FocusStopEvent"), this,
          [this](const EventWrapper& event) {
            device_event_calls_[kFocusStop] = event.GetValueWithDefault(
                kDeviceHash, InputManager::kMaxNumDeviceTypes);
          });
      dispatcher_system->Connect(
          target, Hash(prefix.to_string() + "PressEvent"), this,
          [this](const EventWrapper& event) {
            button_event_calls_[kPress].device = event.GetValueWithDefault(
                kDeviceHash, InputManager::kMaxNumDeviceTypes);
            button_event_calls_[kPress].button = event.GetValueWithDefault(
                kButtonHash, InputManager::kInvalidButton);
          });
      dispatcher_system->Connect(
          target, Hash(prefix.to_string() + "ReleaseEvent"), this,
          [this](const EventWrapper& event) {
            button_event_calls_[kRelease].device = event.GetValueWithDefault(
                kDeviceHash, InputManager::kMaxNumDeviceTypes);
            button_event_calls_[kRelease].button = event.GetValueWithDefault(
                kButtonHash, InputManager::kInvalidButton);
          });
      dispatcher_system->Connect(
          target, Hash(prefix.to_string() + "ClickEvent"), this,
          [this](const EventWrapper& event) {
            button_event_calls_[kClick].device = event.GetValueWithDefault(
                kDeviceHash, InputManager::kMaxNumDeviceTypes);
            button_event_calls_[kClick].button = event.GetValueWithDefault(
                kButtonHash, InputManager::kInvalidButton);
          });
      dispatcher_system->Connect(
          target, Hash(prefix.to_string() + "LongPressEvent"), this,
          [this](const EventWrapper& event) {
            button_event_calls_[kLongPress].device = event.GetValueWithDefault(
                kDeviceHash, InputManager::kMaxNumDeviceTypes);
            button_event_calls_[kLongPress].button = event.GetValueWithDefault(
                kButtonHash, InputManager::kInvalidButton);
          });
    }
  }

  ~InputEventListener() {
    auto* dispatcher = registry_->Get<Dispatcher>();
    if (dispatcher) {
      dispatcher->DisconnectAll(this);
    }
  }

  void Reset() {
    for (size_t index = 0; index < kNumDeviceEventTypes; index++) {
      device_event_calls_[index] = InputManager::kMaxNumDeviceTypes;
    }
    for (size_t index = 0; index < kNumButtonEventTypes; index++) {
      button_event_calls_[index].Reset();
    }
  }

  void ExpectDefaultState() {
    for (size_t index = 0; index < kNumDeviceEventTypes; index++) {
      EXPECT_EQ(device_event_calls_[index], InputManager::kMaxNumDeviceTypes);
    }
    for (size_t index = 0; index < kNumButtonEventTypes; index++) {
      EXPECT_EQ(button_event_calls_[index].device,
                InputManager::kMaxNumDeviceTypes);
      EXPECT_EQ(button_event_calls_[index].button,
                InputManager::kInvalidButton);
    }
  }

 private:
  Entity target_;
  Registry* registry_;
};

std::ostream& operator<<(std::ostream& os, const InputEventListener& a) {
  return os << "device_events: [" << a.device_event_calls_[kFocusStart] << ", "
            << a.device_event_calls_[kFocusStop] << "] "
            << "button_events: [" << a.button_event_calls_[kPress].device << "/"
            << a.button_event_calls_[kPress].button << ", "
            << a.button_event_calls_[kRelease].device << "/"
            << a.button_event_calls_[kRelease].button << ", "
            << a.button_event_calls_[kClick].device << "/"
            << a.button_event_calls_[kClick].button << ", "
            << a.button_event_calls_[kLongPress].device << "/"
            << a.button_event_calls_[kLongPress].button << "] ";
}
TEST_F(InputProcessorTest, PrimaryDevice) {
  // Test that setting and getting the primary device works correctly.
  EXPECT_EQ(input_processor_->GetPrimaryDevice(),
            InputManager::kMaxNumDeviceTypes);
  input_processor_->SetPrimaryDevice(InputManager::kHmd);
  EXPECT_EQ(input_processor_->GetPrimaryDevice(), InputManager::kHmd);
  input_processor_->SetPrimaryDevice(InputManager::kController2);
  EXPECT_EQ(input_processor_->GetPrimaryDevice(), InputManager::kController2);
}

TEST_F(InputProcessorTest, SettingAndGettingInputFocus) {
  // Test that the current and previous InputFocus getters work correctly.
  InputFocus focus1, focus2, focus3;
  focus1.target = 1;
  focus1.device = InputManager::kController;
  focus2.target = 2;
  focus2.device = InputManager::kController;
  focus3.target = 3;
  focus3.device = InputManager::kController;

  EXPECT_EQ(input_processor_->GetInputFocus(InputManager::kController),
            nullptr);
  EXPECT_EQ(input_processor_->GetPreviousFocus(InputManager::kController),
            nullptr);

  input_processor_->UpdateDevice(kDeltaTime, focus1);
  EXPECT_EQ(input_processor_->GetInputFocus(InputManager::kController)->target,
            focus1.target);
  EXPECT_EQ(
      input_processor_->GetPreviousFocus(InputManager::kController)->target,
      kNullEntity);

  input_processor_->UpdateDevice(kDeltaTime, focus2);
  EXPECT_EQ(input_processor_->GetInputFocus(InputManager::kController)->target,
            focus2.target);
  EXPECT_EQ(
      input_processor_->GetPreviousFocus(InputManager::kController)->target,
      focus1.target);

  input_processor_->UpdateDevice(kDeltaTime, focus3);
  EXPECT_EQ(input_processor_->GetInputFocus(InputManager::kController)->target,
            focus3.target);
  EXPECT_EQ(
      input_processor_->GetPreviousFocus(InputManager::kController)->target,
      focus2.target);
}

TEST_F(InputProcessorTest, AnyFocusEvents) {
  // Test that all events are sent out with the Any* prefix, even without any
  // prefixes set up.
  Blueprint blueprint;
  {
    TransformDefT transform;
    blueprint.Write(&transform);
  }
  Entity target = entity_factory_->Create(&blueprint);

  InputEventListener global(registry_.get(), kNullEntity, "Any");
  InputEventListener local(registry_.get(), target, "Any");

  InputFocus focus;
  focus.interactive = true;
  focus.device = InputManager::kController;

  input_processor_->UpdateDevice(kDeltaTime, focus);

  global.ExpectDefaultState();
  local.ExpectDefaultState();

  focus.target = target;
  input_processor_->UpdateDevice(kDeltaTime, focus);

  EXPECT_EQ(global.device_event_calls_[kFocusStart], InputManager::kController);
  EXPECT_EQ(local.device_event_calls_[kFocusStart], InputManager::kController);
  global.device_event_calls_[kFocusStart] = InputManager::kMaxNumDeviceTypes;
  local.device_event_calls_[kFocusStart] = InputManager::kMaxNumDeviceTypes;
  global.ExpectDefaultState();
  local.ExpectDefaultState();

  input_processor_->UpdateDevice(kDeltaTime, focus);

  global.ExpectDefaultState();
  local.ExpectDefaultState();

  focus.target = kNullEntity;
  input_processor_->UpdateDevice(kDeltaTime, focus);

  EXPECT_EQ(global.device_event_calls_[kFocusStop], InputManager::kController);
  EXPECT_EQ(local.device_event_calls_[kFocusStop], InputManager::kController);
  global.device_event_calls_[kFocusStop] = InputManager::kMaxNumDeviceTypes;
  local.device_event_calls_[kFocusStop] = InputManager::kMaxNumDeviceTypes;
  global.ExpectDefaultState();
  local.ExpectDefaultState();
}

TEST_F(InputProcessorTest, AnyClickEvent) {
  const auto button = InputManager::kPrimaryButton;
  const auto device = InputManager::kController;
  // Test that all events are sent out with the Any* prefix, even without any
  // prefixes set up.
  Blueprint blueprint;
  {
    TransformDefT transform;
    blueprint.Write(&transform);
  }
  Entity target = entity_factory_->Create(&blueprint);

  InputEventListener global(registry_.get(), kNullEntity, "Any");
  InputEventListener local(registry_.get(), target, "Any");

  InputFocus focus;
  focus.interactive = true;
  focus.device = InputManager::kController;

  focus.target = target;
  input_processor_->UpdateDevice(kDeltaTime, focus);

  global.Reset();
  local.Reset();

  input_manager_->UpdateButton(device, button, true, false);
  input_manager_->AdvanceFrame(kDeltaTime);
  input_processor_->UpdateDevice(kDeltaTime, focus);

  EXPECT_EQ(global.button_event_calls_[kPress].device, device);
  EXPECT_EQ(global.button_event_calls_[kPress].button, button);
  EXPECT_EQ(local.button_event_calls_[kPress].device, device);
  EXPECT_EQ(local.button_event_calls_[kPress].button, button);
  global.button_event_calls_[kPress].Reset();
  local.button_event_calls_[kPress].Reset();
  global.device_event_calls_[kFocusStart] = InputManager::kMaxNumDeviceTypes;
  local.device_event_calls_[kFocusStart] = InputManager::kMaxNumDeviceTypes;
  global.ExpectDefaultState();
  local.ExpectDefaultState();

  input_manager_->UpdateButton(device, button, true, false);
  input_manager_->AdvanceFrame(kDeltaTime);
  input_processor_->UpdateDevice(kDeltaTime, focus);

  global.ExpectDefaultState();
  local.ExpectDefaultState();

  input_manager_->UpdateButton(device, button, false, false);
  input_manager_->AdvanceFrame(kDeltaTime);
  input_processor_->UpdateDevice(kDeltaTime, focus);

  EXPECT_EQ(global.button_event_calls_[kClick].device, device);
  EXPECT_EQ(global.button_event_calls_[kClick].button, button);
  EXPECT_EQ(local.button_event_calls_[kClick].device, device);
  EXPECT_EQ(local.button_event_calls_[kClick].button, button);
  EXPECT_EQ(global.button_event_calls_[kRelease].device, device);
  EXPECT_EQ(global.button_event_calls_[kRelease].button, button);
  EXPECT_EQ(local.button_event_calls_[kRelease].device, device);
  EXPECT_EQ(local.button_event_calls_[kRelease].button, button);
  global.button_event_calls_[kClick].Reset();
  local.button_event_calls_[kClick].Reset();
  global.button_event_calls_[kRelease].Reset();
  local.button_event_calls_[kRelease].Reset();
  global.ExpectDefaultState();
  local.ExpectDefaultState();
}

TEST_F(InputProcessorTest, AnyLongPressEvent) {
  const auto button = InputManager::kPrimaryButton;
  const auto device = InputManager::kController;
  // Test that all events are sent out with the Any* prefix, even without any
  // prefixes set up.
  Blueprint blueprint;
  {
    TransformDefT transform;
    blueprint.Write(&transform);
  }
  Entity target = entity_factory_->Create(&blueprint);

  InputEventListener global(registry_.get(), kNullEntity, "Any");
  InputEventListener local(registry_.get(), target, "Any");

  InputFocus focus;
  focus.interactive = true;
  focus.device = InputManager::kController;

  focus.target = target;
  input_processor_->UpdateDevice(kDeltaTime, focus);

  global.Reset();
  local.Reset();

  input_manager_->UpdateButton(device, button, true, false);
  input_manager_->AdvanceFrame(kDeltaTime);
  input_processor_->UpdateDevice(kDeltaTime, focus);

  global.Reset();
  local.Reset();

  input_manager_->UpdateButton(device, button, true, false);
  input_manager_->AdvanceFrame(kLongPressTime);
  input_processor_->UpdateDevice(kDeltaTime, focus);

  EXPECT_EQ(global.button_event_calls_[kLongPress].device, device);
  EXPECT_EQ(global.button_event_calls_[kLongPress].button, button);
  EXPECT_EQ(local.button_event_calls_[kLongPress].device, device);
  EXPECT_EQ(local.button_event_calls_[kLongPress].button, button);
  global.button_event_calls_[kLongPress].Reset();
  local.button_event_calls_[kLongPress].Reset();
  global.ExpectDefaultState();
  local.ExpectDefaultState();

  input_manager_->UpdateButton(device, button, false, false);
  input_manager_->AdvanceFrame(kDeltaTime);
  input_processor_->UpdateDevice(kDeltaTime, focus);

  EXPECT_EQ(global.button_event_calls_[kRelease].device, device);
  EXPECT_EQ(global.button_event_calls_[kRelease].button, button);
  EXPECT_EQ(local.button_event_calls_[kRelease].device, device);
  EXPECT_EQ(local.button_event_calls_[kRelease].button, button);
  global.button_event_calls_[kRelease].Reset();
  local.button_event_calls_[kRelease].Reset();
  global.ExpectDefaultState();
  local.ExpectDefaultState();
}

TEST_F(InputProcessorTest, AnyClickFail) {
  const auto button = InputManager::kPrimaryButton;
  const auto device = InputManager::kController;
  // Test that all events are sent out with the Any* prefix, even without any
  // prefixes set up.
  Blueprint blueprint;
  {
    TransformDefT transform;
    blueprint.Write(&transform);
  }
  Entity target = entity_factory_->Create(&blueprint);

  InputEventListener global(registry_.get(), kNullEntity, "Any");
  InputEventListener local(registry_.get(), target, "Any");

  InputFocus focus;
  focus.interactive = true;
  focus.device = InputManager::kController;

  focus.target = target;
  input_processor_->UpdateDevice(kDeltaTime, focus);

  global.Reset();
  local.Reset();

  input_manager_->UpdateButton(device, button, true, false);
  input_manager_->AdvanceFrame(kDeltaTime);
  input_processor_->UpdateDevice(kDeltaTime, focus);

  global.button_event_calls_[kPress].Reset();
  local.button_event_calls_[kPress].Reset();
  global.ExpectDefaultState();
  local.ExpectDefaultState();

  focus.target = kNullEntity;
  input_manager_->UpdateButton(device, button, false, false);
  input_manager_->AdvanceFrame(kDeltaTime);
  input_processor_->UpdateDevice(kDeltaTime, focus);

  // Make sure no click event was sent here, but that the release was.
  EXPECT_EQ(global.button_event_calls_[kRelease].device, device);
  EXPECT_EQ(global.button_event_calls_[kRelease].button, button);
  EXPECT_EQ(local.button_event_calls_[kRelease].device, device);
  EXPECT_EQ(local.button_event_calls_[kRelease].button, button);
  global.button_event_calls_[kRelease].Reset();
  local.button_event_calls_[kRelease].Reset();
  global.device_event_calls_[kFocusStop] = InputManager::kMaxNumDeviceTypes;
  local.device_event_calls_[kFocusStop] = InputManager::kMaxNumDeviceTypes;
  global.ExpectDefaultState();
  local.ExpectDefaultState();
}

TEST_F(InputProcessorTest, AnyLongPressFail) {
  const auto button = InputManager::kPrimaryButton;
  const auto device = InputManager::kController;
  // Test that all events are sent out with the Any* prefix, even without any
  // prefixes set up.
  Blueprint blueprint;
  {
    TransformDefT transform;
    blueprint.Write(&transform);
  }
  Entity target = entity_factory_->Create(&blueprint);

  InputEventListener global(registry_.get(), kNullEntity, "Any");
  InputEventListener local(registry_.get(), target, "Any");

  InputFocus focus;
  focus.interactive = true;
  focus.device = InputManager::kController;

  focus.target = target;
  input_processor_->UpdateDevice(kDeltaTime, focus);

  global.Reset();
  local.Reset();

  input_manager_->UpdateButton(device, button, true, false);
  input_manager_->AdvanceFrame(kDeltaTime);
  input_processor_->UpdateDevice(kDeltaTime, focus);

  global.Reset();
  local.Reset();

  focus.target = kNullEntity;
  input_manager_->UpdateButton(device, button, true, false);
  input_manager_->AdvanceFrame(kLongPressTime);
  input_processor_->UpdateDevice(kDeltaTime, focus);

  global.device_event_calls_[kFocusStop] = InputManager::kMaxNumDeviceTypes;
  local.device_event_calls_[kFocusStop] = InputManager::kMaxNumDeviceTypes;

  // Ensure no long press event was sent
  global.ExpectDefaultState();
  local.ExpectDefaultState();

  input_manager_->UpdateButton(device, button, false, false);
  input_manager_->AdvanceFrame(kDeltaTime);
  input_processor_->UpdateDevice(kDeltaTime, focus);

  EXPECT_EQ(global.button_event_calls_[kRelease].device, device);
  EXPECT_EQ(global.button_event_calls_[kRelease].button, button);
  EXPECT_EQ(local.button_event_calls_[kRelease].device, device);
  EXPECT_EQ(local.button_event_calls_[kRelease].button, button);
  global.button_event_calls_[kRelease].Reset();
  local.button_event_calls_[kRelease].Reset();
  global.ExpectDefaultState();
  local.ExpectDefaultState();
}

TEST_F(InputProcessorTest, DevicePrefixes) {
  input_processor_->SetPrefix(InputManager::kController, "Controller1");
  input_processor_->SetPrefix(InputManager::kController2, "Controller2");
  input_processor_->SetPrefix(InputManager::kHmd, "Hmd");

  // Test that all events sent to various devices are sent with the correct
  // prefix.
  Blueprint blueprint;
  {
    TransformDefT transform;
    blueprint.Write(&transform);
  }
  Entity target = entity_factory_->Create(&blueprint);

  InputEventListener c1(registry_.get(), target, "Controller1");
  InputEventListener c2(registry_.get(), target, "Controller2");
  InputEventListener h(registry_.get(), target, "Hmd");

  InputFocus focus;

  focus.interactive = true;
  focus.device = InputManager::kController;
  focus.target = target;
  input_processor_->UpdateDevice(kDeltaTime, focus);

  focus.interactive = false;
  focus.device = InputManager::kController2;
  focus.target = kNullEntity;
  input_processor_->UpdateDevice(kDeltaTime, focus);

  focus.device = InputManager::kHmd;
  input_processor_->UpdateDevice(kDeltaTime, focus);

  EXPECT_EQ(c1.device_event_calls_[kFocusStart], InputManager::kController);
  c1.device_event_calls_[kFocusStart] = InputManager::kMaxNumDeviceTypes;
  c1.ExpectDefaultState();
  c2.ExpectDefaultState();
  h.ExpectDefaultState();

  focus.interactive = false;
  focus.device = InputManager::kController;
  focus.target = target;
  input_processor_->UpdateDevice(kDeltaTime, focus);

  focus.interactive = true;
  focus.device = InputManager::kController2;
  focus.target = target;
  input_processor_->UpdateDevice(kDeltaTime, focus);

  focus.interactive = false;
  focus.device = InputManager::kHmd;
  focus.target = kNullEntity;
  input_processor_->UpdateDevice(kDeltaTime, focus);

  EXPECT_EQ(c1.device_event_calls_[kFocusStop], InputManager::kController);
  EXPECT_EQ(c2.device_event_calls_[kFocusStart], InputManager::kController2);
  c1.device_event_calls_[kFocusStop] = InputManager::kMaxNumDeviceTypes;
  c2.device_event_calls_[kFocusStart] = InputManager::kMaxNumDeviceTypes;
  c1.ExpectDefaultState();
  c2.ExpectDefaultState();
  h.ExpectDefaultState();
}

TEST_F(InputProcessorTest, SharedDevicePrefixes) {
  const auto button = InputManager::kPrimaryButton;
  const auto device = InputManager::kController;
  const auto device2 = InputManager::kController2;

  input_processor_->SetPrefix(device, button, "shared");
  input_processor_->SetPrefix(device2, button, "shared");

  // Test that when two devices share the same prefix, events from one device
  // or the other are sent out correctly.

  InputEventListener listener(registry_.get(), kNullEntity, "shared");
  InputFocus focus;

  InputManager::DeviceParams params;
  params.num_buttons = 2;
  input_manager_->ConnectDevice(device2, params);

  input_manager_->UpdateButton(device, button, false, false);
  input_manager_->UpdateButton(device2, button, false, false);
  input_manager_->AdvanceFrame(kDeltaTime);

  focus.device = device;
  input_processor_->UpdateDevice(kDeltaTime, focus);
  focus.device = device2;
  input_processor_->UpdateDevice(kDeltaTime, focus);

  listener.ExpectDefaultState();


  input_manager_->UpdateButton(device, button, true, false);
  input_manager_->UpdateButton(device2, button, false, false);
  input_manager_->AdvanceFrame(kDeltaTime);

  focus.device = device;
  input_processor_->UpdateDevice(kDeltaTime, focus);
  focus.device = device2;
  input_processor_->UpdateDevice(kDeltaTime, focus);

  EXPECT_EQ(listener.button_event_calls_[kPress].device, device);
  listener.button_event_calls_[kPress].Reset();
  listener.ExpectDefaultState();

  input_manager_->UpdateButton(device, button, true, false);
  input_manager_->UpdateButton(device2, button, true, false);
  input_manager_->AdvanceFrame(kDeltaTime);

  focus.device = device;
  input_processor_->UpdateDevice(kDeltaTime, focus);
  focus.device = device2;
  input_processor_->UpdateDevice(kDeltaTime, focus);

  EXPECT_EQ(listener.button_event_calls_[kPress].device, device2);
  listener.button_event_calls_[kPress].Reset();
  listener.ExpectDefaultState();
}
}  // namespace
}  // namespace lull
