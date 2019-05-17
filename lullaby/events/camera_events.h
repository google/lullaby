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

#ifndef LULLABY_EVENTS_CAMERA_EVENTS_H_
#define LULLABY_EVENTS_CAMERA_EVENTS_H_

#include "lullaby/util/hash.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Event the app should send to set camera display geometry.  Camera
// implementations such as ArCamera or MutableCamera will listen to this event.
struct SetCameraDisplayGeometryEvent {
  SetCameraDisplayGeometryEvent() {}
  SetCameraDisplayGeometryEvent(int width, int height, int rotation)
      : width(width), height(height), rotation(rotation) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&width, ConstHash("width"));
    archive(&height, ConstHash("height"));
    archive(&rotation, ConstHash("rotation"));
  }

  int width;
  int height;
  // Should correspond to android.view.Surface.ROTATION_*.
  int rotation;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::SetCameraDisplayGeometryEvent);

#endif  // LULLABY_EVENTS_CAMERA_EVENTS_H_
