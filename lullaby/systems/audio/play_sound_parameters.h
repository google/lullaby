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

#ifndef LULLABY_SYSTEMS_AUDIO_PLAY_SOUND_PARAMETERS_H_
#define LULLABY_SYSTEMS_AUDIO_PLAY_SOUND_PARAMETERS_H_

#include "lullaby/generated/audio_playback_types_generated.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Parameters to control how a sound is played.
struct PlaySoundParameters {
  // Describes how the sound should be retrieved and played. Because sounds can
  // be loaded asynchronously, this may be used to determine when a sound is
  // played.
  AudioPlaybackType playback_type = AudioPlaybackType::AudioPlaybackType_Stream;

  // The volume level of the sound in the range [0, +inf). Values higher than 1
  // indicate gain.
  float volume = 1.f;

  // Whether or not the sound should loop on completion.
  bool loop = false;

  // What, if any, spatialization this sound should be played with.
  AudioSourceType source_type = AudioSourceType::AudioSourceType_Spatialized;

  // The directivity constant "alpha". This value is a weighting balance
  // between a figure 8 pattern and omnidirectional pattern for source
  // emission. Range of [0, 1], with a value of 0.5 results in a cardioid
  // pattern.
  // This value will be only be checked when source_type = Spatialized. If it
  // is not in the range [0, 1], it will be ignored and directivity will not be
  // set for this sound.
  float spatial_directivity_alpha = -1.0;

  // The directivity constant "order". This value is applied to computed
  // directivity. Higher values will result in narrower and sharper directivity
  // patterns. Range of [1, inf).
  // This value will be only be checked when source_type = Spatialized. If it
  // is not in the range [1, inf), it will be ignored and directivity will not
  // be set for this sound.
  float spatial_directivity_order = -1.0;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&playback_type, ConstHash("playback_type"));
    archive(&volume, ConstHash("volume"));
    archive(&loop, ConstHash("loop"));
    archive(&source_type, ConstHash("source_type"));
    archive(&spatial_directivity_alpha, ConstHash("spatial_directivity_alpha"));
    archive(&spatial_directivity_order, ConstHash("spatial_directivity_order"));
  }
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::PlaySoundParameters);

#endif  // LULLABY_SYSTEMS_AUDIO_PLAY_SOUND_PARAMETERS_H_
