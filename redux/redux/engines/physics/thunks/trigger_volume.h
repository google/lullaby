/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#ifndef REDUX_ENGINES_PHYSICS_THUNKS_TRIGGER_VOLUME_H_
#define REDUX_ENGINES_PHYSICS_THUNKS_TRIGGER_VOLUME_H_

#include "redux/engines/physics/trigger_volume.h"

namespace redux {

// Thunk functions to call the actual implementation.
void TriggerVolume::Activate() { Upcast(this)->Activate(); }
void TriggerVolume::Deactivate() { Upcast(this)->Deactivate(); }
bool TriggerVolume::IsActive() const { return Upcast(this)->IsActive(); }
void TriggerVolume::SetTransform(const Transform& transform) {
  Upcast(this)->SetTransform(transform);
}
vec3 TriggerVolume::GetPosition() const { return Upcast(this)->GetPosition(); }
quat TriggerVolume::GetRotation() const { return Upcast(this)->GetRotation(); }

}  // namespace redux

#endif  // REDUX_ENGINES_PHYSICS_THUNKS_TRIGGER_VOLUME_H_
