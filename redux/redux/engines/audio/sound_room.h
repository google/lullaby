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

#ifndef REDUX_ENGINES_AUDIO_SOUND_ROOM_H_
#define REDUX_ENGINES_AUDIO_SOUND_ROOM_H_

#include "redux/modules/audio/enums.h"
#include "redux/modules/math/vector.h"

namespace redux {

// Properties for a shoebox room that affect how sound propogates when the
// listener is inside the room.
struct SoundRoom {
  enum WallTypes {
    kLeftWall,
    kRightWall,
    kBottomWall,
    kTopWall,
    kFrontWall,
    kBackWall,
    kNumWalls,
  };

  // Size of the shoebox room in world space.
  vec3 size = vec3::Zero();

  // Material name of each surface of the room.
  AudioSurfaceMaterial surface_materials[kNumWalls] = {
      AudioSurfaceMaterial::Transparent, AudioSurfaceMaterial::Transparent,
      AudioSurfaceMaterial::Transparent, AudioSurfaceMaterial::Transparent,
      AudioSurfaceMaterial::Transparent, AudioSurfaceMaterial::Transparent,
  };

  // User defined uniform scaling factor for all reflection coefficients.
  float reflection_scalar = 1.0f;

  // User defined reverb tail gain multiplier.
  float reverb_gain = 1.0f;

  // Adjusts the reverberation time across all frequency bands. RT60 values
  // are multiplied by this factor. Has no effect when set to 1.0f.
  float reverb_time = 1.0f;

  // Controls the slope of a line from the lowest to the highest RT60 values
  // (increases high frequency RT60s when positive, decreases when negative).
  // Has no effect when set to 0.0f.
  float reverb_brightness = 0.0f;
};

}  // namespace redux

#endif  // REDUX_ENGINES_AUDIO_SOUND_ROOM_H_
