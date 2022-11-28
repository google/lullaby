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

#ifndef REDUX_MODULES_AUDIO_ENUMS_H_
#define REDUX_MODULES_AUDIO_ENUMS_H_

#include <stddef.h>  // for types missing from the autogen file.
#include <stdint.h>  // for types missing from the autogen file.

#include "redux/modules/audio/audio_enums_generated.h"
#include "redux/modules/base/typeid.h"

namespace redux {

inline const char* ToString(AudioSurfaceMaterial e) {
  return EnumNameAudioSurfaceMaterial(e);
}

inline const char* ToString(SoundType e) { return EnumNameSoundType(e); }

}  // namespace redux

REDUX_SETUP_TYPEID(redux::AudioSurfaceMaterial);
REDUX_SETUP_TYPEID(redux::SoundType);

#endif  // REDUX_MODULES_AUDIO_ENUMS_H_
