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
#include "vr/gvr/capi/include/gvr_types.h"

namespace lull {

// Parameters to control how a sound is played.
struct PlaySoundParameters {
  // Describes how the sound should be treated if playback is requested before
  // preloading is complete.
  AudioPlaybackType playback_type =
      AudioPlaybackType::AudioPlaybackType_PlayWhenReady;

  // The volume level of the sound in the range [0, +inf). Values higher than 1
  // indicate gain.
  float volume = 1.f;

  // Whether or not the sound should loop on completion.
  bool loop = false;

  // What, if any, spatialization this sound should be played with.
  AudioSourceType source_type = AudioSourceType::AudioSourceType_SoundObject;

  // The directivity constant "alpha". This value is a weighting balance
  // between a figure 8 pattern and omnidirectional pattern for source
  // emission. Range of [0, 1], with a value of 0.5 results in a cardioid
  // pattern.
  // This value will be only be checked when source_type = SoundObject. If it
  // is not in the range [0, 1], it will be ignored and directivity will not be
  // set for this sound.
  float spatial_directivity_alpha = -1.0;

  // The directivity constant "order". This value is applied to computed
  // directivity. Higher values will result in narrower and sharper directivity
  // patterns. Range of [1, inf).
  // This value will be only be checked when source_type = SoundObject. If it
  // is not in the range [1, inf), it will be ignored and directivity will not
  // be set for this sound.
  float spatial_directivity_order = -1.0;

  // The spatial rolloff method. This value will only be applied when
  // source_type = SoundObject and |spatial_rolloff_min_distance| and
  // |spatial_rolloff_max_distance| are valid values. Otherwise, a default
  // rolloff model is applied.
  gvr::AudioRolloffMethod spatial_rolloff_method =
      gvr::AudioRolloffMethod::GVR_AUDIO_ROLLOFF_LOGARITHMIC;

  // The minimum and maximum distances for sound object rolloff. Setting both
  // to values > 0 will apply the |spatial_rolloff_method| with the specified
  // distances. Otherwise, a default rolloff model is applied.
  float spatial_rolloff_min_distance = -1.0;
  float spatial_rolloff_max_distance = -1.0;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&playback_type, ConstHash("playback_type"));
    archive(&volume, ConstHash("volume"));
    archive(&loop, ConstHash("loop"));
    archive(&source_type, ConstHash("source_type"));
    archive(&spatial_directivity_alpha, ConstHash("spatial_directivity_alpha"));
    archive(&spatial_directivity_order, ConstHash("spatial_directivity_order"));
    archive(&spatial_rolloff_method, ConstHash("spatial_rolloff_method"));
    archive(&spatial_rolloff_min_distance,
            ConstHash("spatial_rolloff_min_distance"));
    archive(&spatial_rolloff_max_distance,
            ConstHash("spatial_rolloff_max_distance"));
  }
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::PlaySoundParameters);

#endif  // LULLABY_SYSTEMS_AUDIO_PLAY_SOUND_PARAMETERS_H_
