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

#include "lullaby/modules/input_processor/touchscreen_gestures.h"

#include "lullaby/modules/input/input_focus.h"
#include "lullaby/modules/input_processor/gesture.h"

namespace lull {
namespace {
const float kInchesToCM = 2.54f;
const float kDragDeltaCM = 0.1f * kInchesToCM;
const float kDragDeltaSquared = kDragDeltaCM * kDragDeltaCM;
const float kTwistThreshold = 5.0f * kDegreesToRadians;

const float kPinchDirectionTheshold = cosf(30.f * kDegreesToRadians);
const float kPinchDelta = 0.05f * kInchesToCM;

float CalculateDeltaRotation(mathfu::vec2 current_position1,
                             mathfu::vec2 current_position2,
                             mathfu::vec2 previous_position1,
                             mathfu::vec2 previous_position2) {
  const mathfu::vec2 current_direction =
      (current_position1 - current_position2).Normalized();
  const mathfu::vec2 previous_direction =
      (previous_position1 - previous_position2).Normalized();
  const float sign = (previous_direction.x * current_direction.y -
                      previous_direction.y * current_direction.x) > 0
                         ? 1
                         : -1;
  return mathfu::vec2::Angle(current_direction, previous_direction) * sign;
}

}  // namespace

GesturePtr OneFingerDragRecognizer::TryStart(InputManager::DeviceType device,
                                             InputManager::TouchpadId touchpad,
                                             Gesture::TouchIdSpan ids) {
  // If the touch is already owned, ignore it.
  if (input_processor_->GetTouchOwner(device, touchpad, ids[0]) != nullptr) {
    return nullptr;
  }

  const mathfu::vec2 start_pos =
      touchpad_size_cm_ *
      input_manager_->GetTouchGestureOrigin(device, touchpad, ids[0]);
  const mathfu::vec2 cur_pos =
      touchpad_size_cm_ *
      input_manager_->GetTouchLocation(device, touchpad, ids[0]);
  const mathfu::vec2 delta_cm = (start_pos - cur_pos);

  DCHECK(start_pos != InputManager::kInvalidTouchLocation);
  DCHECK(cur_pos != InputManager::kInvalidTouchLocation);

  if (delta_cm.LengthSquared() >= kDragDeltaSquared) {
    return std::make_shared<OneFingerDrag>(callback_);
  }
  return nullptr;
}

Gesture::State OneFingerDragRecognizer::OneFingerDrag::AdvanceFrame(
    const Clock::duration& delta_time) {
  if (state_ == kCanceled) {
    // Callback should revert changes.
    callback_(state_, target_, InputManager::kInvalidTouchLocation);
    return state_;
  }
  if (!input_manager_->IsValidTouch(device_, touchpad_, ids_[0])) {
    // Touch has been released, so end the gesture.
    state_ = kEnding;
    // Set state before the callback, since this will be the last call.
    callback_(state_, target_, InputManager::kInvalidTouchLocation);
    return state_;
  }
  // Gesture is ongoing.
  const mathfu::vec2 cur_pos =
      input_manager_->GetTouchLocation(device_, touchpad_, ids_[0]);
  callback_(state_, target_, cur_pos);
  // Set state after the callback, so that the first frame will have
  // kStarting.
  state_ = kRunning;
  return state_;
}

GesturePtr TwistRecognizer::TryStart(InputManager::DeviceType device,
                                     InputManager::TouchpadId touchpad,
                                     Gesture::TouchIdSpan ids) {
  // If the touch is already owned, ignore it.
  if (input_processor_->GetTouchOwner(device, touchpad, ids[0]) != nullptr ||
      input_processor_->GetTouchOwner(device, touchpad, ids[1]) != nullptr) {
    return nullptr;
  }

  const mathfu::vec2 delta1 =
      input_manager_->GetTouchDelta(device, touchpad, ids[0]);
  const mathfu::vec2 delta2 =
      input_manager_->GetTouchDelta(device, touchpad, ids[1]);
  // Make sure both touches are moving.
  if (delta1.LengthSquared() < 0.00001f || delta2.LengthSquared() < 0.00001f) {
    return nullptr;
  }

  // all positions should be in cm before calculating rotation.
  const mathfu::vec2 start_pos1 =
      touchpad_size_cm_ *
      input_manager_->GetTouchGestureOrigin(device, touchpad, ids[0]);
  const mathfu::vec2 cur_pos1 =
      touchpad_size_cm_ *
      input_manager_->GetTouchLocation(device, touchpad, ids[0]);

  const mathfu::vec2 start_pos2 =
      touchpad_size_cm_ *
      input_manager_->GetTouchGestureOrigin(device, touchpad, ids[1]);
  const mathfu::vec2 cur_pos2 =
      touchpad_size_cm_ *
      input_manager_->GetTouchLocation(device, touchpad, ids[1]);

  DCHECK(start_pos1 != InputManager::kInvalidTouchLocation);
  DCHECK(start_pos2 != InputManager::kInvalidTouchLocation);
  DCHECK(cur_pos1 != InputManager::kInvalidTouchLocation);
  DCHECK(cur_pos2 != InputManager::kInvalidTouchLocation);

  const float rotation =
      CalculateDeltaRotation(cur_pos1, cur_pos2, start_pos1, start_pos2);
  if (std::abs(rotation) > kTwistThreshold) {
    return std::make_shared<Twist>(callback_);
  }
  return nullptr;
}

Gesture::State TwistRecognizer::Twist::AdvanceFrame(
    const Clock::duration& delta_time) {
  if (state_ == kCanceled) {
    // Callback should revert changes.
    callback_(state_, target_, 0.0f);
    return state_;
  }
  if (!input_manager_->IsValidTouch(device_, touchpad_, ids_[0]) ||
      !input_manager_->IsValidTouch(device_, touchpad_, ids_[1])) {
    // A touch has been released, so end the gesture.
    state_ = kEnding;
    // Set state before the callback, since this will be the last call.
    callback_(state_, target_, 0.0f);
    return state_;
  }

  // Gesture is ongoing.
  const mathfu::vec2 prev_pos1 =
      touchpad_size_cm_ *
      input_manager_->GetPreviousTouchLocation(device_, touchpad_, ids_[0]);
  const mathfu::vec2 cur_pos1 =
      touchpad_size_cm_ *
      input_manager_->GetTouchLocation(device_, touchpad_, ids_[0]);

  const mathfu::vec2 prev_pos2 =
      touchpad_size_cm_ *
      input_manager_->GetPreviousTouchLocation(device_, touchpad_, ids_[1]);
  const mathfu::vec2 cur_pos2 =
      touchpad_size_cm_ *
      input_manager_->GetTouchLocation(device_, touchpad_, ids_[1]);

  const float rotation =
      CalculateDeltaRotation(cur_pos1, cur_pos2, prev_pos1, prev_pos2);
  callback_(state_, target_, rotation);
  // Set state after the callback, so that the first frame will have
  // kStarting.
  state_ = kRunning;
  return state_;
}

GesturePtr PinchRecognizer::TryStart(InputManager::DeviceType device,
                                     InputManager::TouchpadId touchpad,
                                     Gesture::TouchIdSpan ids) {
  // If the touch is already owned, ignore it.
  if (input_processor_->GetTouchOwner(device, touchpad, ids[0]) != nullptr ||
      input_processor_->GetTouchOwner(device, touchpad, ids[1]) != nullptr) {
    return nullptr;
  }

  const mathfu::vec2 delta1 = touchpad_size_cm_ * input_manager_->GetTouchDelta(
                                                      device, touchpad, ids[0]);
  const mathfu::vec2 delta2 = touchpad_size_cm_ * input_manager_->GetTouchDelta(
                                                      device, touchpad, ids[1]);

  const mathfu::vec2 start_pos1 =
      touchpad_size_cm_ *
      input_manager_->GetTouchGestureOrigin(device, touchpad, ids[0]);
  const mathfu::vec2 start_pos2 =
      touchpad_size_cm_ *
      input_manager_->GetTouchGestureOrigin(device, touchpad, ids[1]);

  const mathfu::vec2 cur_pos1 =
      touchpad_size_cm_ *
      input_manager_->GetTouchLocation(device, touchpad, ids[0]);
  const mathfu::vec2 cur_pos2 =
      touchpad_size_cm_ *
      input_manager_->GetTouchLocation(device, touchpad, ids[1]);

  DCHECK(start_pos1 != InputManager::kInvalidTouchLocation);
  DCHECK(start_pos2 != InputManager::kInvalidTouchLocation);
  DCHECK(cur_pos1 != InputManager::kInvalidTouchLocation);
  DCHECK(cur_pos2 != InputManager::kInvalidTouchLocation);

  const mathfu::vec2 first_to_second = start_pos1 - start_pos2;
  const mathfu::vec2 first_to_second_dir = first_to_second.Normalized();

  const float dot1 =
      mathfu::vec2::DotProduct(delta1.Normalized(), -first_to_second_dir);
  const float dot2 =
      mathfu::vec2::DotProduct(delta2.Normalized(), first_to_second_dir);

  // check that if a touch is moving, it's either moving towards or away from
  // the other touch.
  if (delta1.LengthSquared() < 0.005f &&
      std::abs(dot1) < kPinchDirectionTheshold) {
    return nullptr;
  }

  if (delta2.LengthSquared() < 0.005f &&
      std::abs(dot2) < kPinchDirectionTheshold) {
    return nullptr;
  }

  const float start_gap = first_to_second.Length();
  const float cur_gap = (cur_pos1 - cur_pos2).Length();
  if (std::abs(start_gap - cur_gap) >= kPinchDelta) {
    return std::make_shared<Pinch>(callback_);
  }
  return nullptr;
}

void PinchRecognizer::Pinch::Initialize() {
  const mathfu::vec2 cur_pos1 =
      touchpad_size_cm_ *
      input_manager_->GetTouchLocation(device_, touchpad_, ids_[0]);
  const mathfu::vec2 cur_pos2 =
      touchpad_size_cm_ *
      input_manager_->GetTouchLocation(device_, touchpad_, ids_[1]);
  start_gap_ = (cur_pos1 - cur_pos2).Length();
}

Gesture::State PinchRecognizer::Pinch::AdvanceFrame(
    const Clock::duration& delta_time) {
  if (state_ == kCanceled) {
    // Callback should revert changes.
    callback_(state_, target_, 1.0f);
    return state_;
  }
  if (!input_manager_->IsValidTouch(device_, touchpad_, ids_[0]) ||
      !input_manager_->IsValidTouch(device_, touchpad_, ids_[1])) {
    // A touch has been released, so end the gesture.
    state_ = kEnding;
    // Set state before the callback, since this will be the last call.
    callback_(state_, target_, 1.0f);
    return state_;
  }
  // Gesture is ongoing.
  const mathfu::vec2 cur_pos1 =
      touchpad_size_cm_ *
      input_manager_->GetTouchLocation(device_, touchpad_, ids_[0]);
  const mathfu::vec2 cur_pos2 =
      touchpad_size_cm_ *
      input_manager_->GetTouchLocation(device_, touchpad_, ids_[1]);

  const float cur_gap = (cur_pos1 - cur_pos2).Length();

  const float ratio = cur_gap / start_gap_;

  callback_(state_, target_, ratio);
  // Set state after the callback, so that the first frame will have
  // kStarting.
  state_ = kRunning;
  return state_;
}

}  // namespace lull
