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

#ifndef REDUX_ENGINES_AUDIO_SOUND_H_
#define REDUX_ENGINES_AUDIO_SOUND_H_

#include <memory>

#include "redux/modules/audio/enums.h"
#include "redux/modules/math/transform.h"

namespace redux {

// A sound that is being played by the AudioEngine.
class Sound {
 public:
  virtual ~Sound() = default;

  Sound(const Sound&) = delete;
  Sound& operator=(const Sound&) = delete;

  // Models used for distance attenuation.
  enum DistanceRolloffModel {
    NoRollof,
    LinearRolloff,
    LogarithmicRolloff,
  };

  // Returns true if the Sound is valid (i.e. loaded in the AudioEngine).
  bool IsValid() const;

  // Resumes (or starts) playing the sound.
  void Resume();

  // Pauses the sound that is playing.
  void Pause();

  // Stops the sound from playing, effectivey invalidating it.
  void Stop();

  // Sets the volume of the sound, ranging from from 0 (mute) to 1 (max).
  void SetVolume(float volume);

  // Returns true if the Sound is playing.
  bool IsPlaying() const;

  // Sets the position and rotation of the sound.
  void SetTransform(const Transform& transform);

  // Sets the directivity of the sound.
  void SetDirectivitiy(float alpha, float order);

  // Sets the distance rolloff model for the sound and the distances at which
  // to apply the model.
  void SetDistanceRolloffModel(DistanceRolloffModel rolloff, float min_distance,
                               float max_distance);

 protected:
  explicit Sound(SoundType type) : type_(type) {}
  SoundType type_;
};

using SoundPtr = std::shared_ptr<Sound>;

}  // namespace redux

#endif  // REDUX_ENGINES_AUDIO_SOUND_H_
