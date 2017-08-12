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

#include "lullaby/modules/input/input_processor.h"

#include "lullaby/events/input_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/util/bits.h"

namespace lull {

namespace {
template <typename PressEvent, typename ReleaseEvent, typename ClickEvent,
          typename LongPressEvent, typename LongClickEvent>
void ProcessEventsForButton(InputManager* input, Dispatcher* dispatcher,
                            InputManager::DeviceType device,
                            InputManager::ButtonId button) {
  if (!input->HasButton(device, button)) {
    return;
  }
  const InputManager::ButtonState state = input->GetButtonState(device, button);
  if (CheckBit(state, InputManager::kJustReleased)) {
    dispatcher->Send(ReleaseEvent());
    if (!CheckBit(state, InputManager::kLongPressed)) {
      // Only send click if the press-release time is short.
      dispatcher->Send(ClickEvent());
    } else {
      dispatcher->Send(LongClickEvent());
    }
  } else if (CheckBit(state, InputManager::kJustLongPressed)) {
    dispatcher->Send(LongPressEvent());
  }
  if (CheckBit(state, InputManager::kJustPressed)) {
    dispatcher->Send(PressEvent());
  }
}
}  // namespace

void ProcessEventsForDevice(Registry* registry,
                            InputManager::DeviceType device) {
  auto* input = registry->Get<lull::InputManager>();
  auto* dispatcher = registry->Get<lull::Dispatcher>();
  if (input->IsConnected(device)) {
    ProcessEventsForButton<PrimaryButtonPress, PrimaryButtonRelease,
                           PrimaryButtonClick, PrimaryButtonLongPress,
                           PrimaryButtonLongClick>(
        input, dispatcher, device, InputManager::kPrimaryButton);

    ProcessEventsForButton<SecondaryButtonPress, SecondaryButtonRelease,
                           SecondaryButtonClick, SecondaryButtonLongPress,
                           SecondaryButtonLongClick>(
        input, dispatcher, device, InputManager::kSecondaryButton);

    ProcessEventsForButton<SystemButtonPress, SystemButtonRelease,
                           SystemButtonClick, SystemButtonLongPress,
                           SystemButtonLongClick>(
        input, dispatcher, device, InputManager::kRecenterButton);
  }
}

}  // namespace lull
