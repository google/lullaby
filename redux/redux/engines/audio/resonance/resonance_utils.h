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

#ifndef REDUX_ENGINES_AUDIO_RESONANCE_RESONANCE_UTILS_H_
#define REDUX_ENGINES_AUDIO_RESONANCE_RESONANCE_UTILS_H_

#include "redux/engines/audio/sound.h"
#include "redux/engines/audio/sound_room.h"
#include "redux/modules/audio/enums.h"
#include "resonance_audio/api/resonance_audio_api.h"
#include "platforms/common/room_properties.h"
#include "redux/modules/audio/audio_reader.h"

namespace redux {

// Returns the vraudio MaterialName matching the AudioSurfaceMaterial.
vraudio::MaterialName ToResonance(AudioSurfaceMaterial type);

// Returns the vraudio DistanceRolloffModel matching the DistanceRolloffModel.
vraudio::DistanceRolloffModel ToResonance(Sound::DistanceRolloffModel model);

// Returns the vraudio RoomPropereties matching the SoundRoom parameters.
vraudio::RoomProperties ToResonance(const SoundRoom& room, const vec3& position,
                                    const quat& rotation);
}  // namespace redux

#endif  // REDUX_ENGINES_AUDIO_RESONANCE_RESONANCE_UTILS_H_
