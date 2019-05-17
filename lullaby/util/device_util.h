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

#ifndef LULLABY_UTIL_DEVICE_UTIL_H_
#define LULLABY_UTIL_DEVICE_UTIL_H_

#include "lullaby/modules/input/device_profile.h"
#include "lullaby/util/math.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Key for use with GetDeviceInfo() to get a lull::Ray that stores the ray in
// device local space that should be used as a 'forward' ray when selecting
// entities in the scene.

static constexpr HashValue kSelectionRayHash = ConstHash("SelectionRay");


// //lullaby/modules/input/input_manager_util.cc")
constexpr float kDaydreamControllerErgoAngleRadians = -0.26f;
const mathfu::vec3 kDaydreamControllerRayOrigin(0.0f, -0.01f, -0.06f);
const mathfu::vec3 kDaydreamControllerRayDirection =
    mathfu::quat::FromAngleAxis(kDaydreamControllerErgoAngleRadians,
                                mathfu::kAxisX3f) *
    -mathfu::kAxisZ3f;
const Ray kDaydreamControllerSelectionRay(kDaydreamControllerRayOrigin,
                                          kDaydreamControllerRayDirection);

// Shader Uniform names
extern const char kControllerButtonUVRectsUniform[];
extern const char kControllerButtonColorsUniform[];
extern const char kControllerBatteryUVRectUniform[];
extern const char kControllerBatteryUVOffsetUniform[];
extern const char kControllerTouchpadRectUniform[];
extern const char kControllerTouchColorUniform[];
extern const char kControllerTouchPositionUniform[];
extern const char kControllerTouchRadiusSquaredUniform[];

// Shader Constants
constexpr uint8_t kControllerMaxTouches = 1;
constexpr uint8_t kControllerMaxColoredButtons = 20;
constexpr uint8_t kControllerMaxBones = 20;


// Return a profile for a given headset model.
// Return CardboardHeadset profile if nothing matches
DeviceProfile GetDeviceProfileForHeadsetModel(const HashValue hash);

// Daydream controller settings.
DeviceProfile GetDaydreamControllerProfile();


// Cardboard Headset settings.
DeviceProfile GetCardboardHeadsetProfile();

// Daydream Headset settings.
DeviceProfile GetDaydreamHeadsetProfile();


// Fake headset for AR.
DeviceProfile GetARHeadsetProfile();

}  // namespace lull

#endif  // LULLABY_UTIL_DEVICE_UTIL_H_
