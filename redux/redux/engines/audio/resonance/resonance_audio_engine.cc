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

#include "redux/engines/audio/resonance/resonance_audio_engine.h"

#include "redux/engines/audio/resonance/audio_stream_renderer.h"
#include "redux/engines/audio/resonance/audio_asset_stream.h"
#include "redux/engines/audio/resonance/resonance_sound.h"
#include "redux/engines/audio/resonance/resonance_utils.h"
#include "redux/engines/platform/device_manager.h"
#include "redux/modules/base/choreographer.h"
#include "redux/modules/base/static_registry.h"
#include "resonance_audio/dsp/channel_converter.h"
#include "resonance_audio/graph/resonance_audio_api_impl.h"
#include "platforms/common/room_effects_utils.h"
#include "resonance_audio/utils/planar_interleaved_conversion.h"
#include "resonance_audio/utils/sample_type_conversion.h"

namespace redux {

// Flag to enable a separate audio processing thread.
static const bool kCreateAudioThread = true;

// Size of FIFO to transmit buffers between audio and processing thread.
#if defined(__ANDROID__)
static const size_t kAudioThreadFifoBufferSize = 4;
#else
const size_t kAudioThreadFifoBufferSize = 1;
#endif

// Maximum waiting period until FIFO slots are available. It is set to a
// duration significantly larger than the duration of a single buffer to avoid
// the output of silence buffers when no processed audio buffer is available in
// the audio callback.
static const size_t kAudioThreadFifoMaxWaitTimeMs = 100;

// Maximum number of sources that can be simultaneously created.
static const size_t kMaxNumberOfSoundSources = 512;

ResonanceAudioEngine::ResonanceAudioEngine(Registry* registry)
    : AudioEngine(registry),
      resonance_(nullptr),
      audio_running_(false),
      audio_thread_task_queue_(kMaxNumberOfSoundSources) {
  auto* choreographer = registry->Get<Choreographer>();
  if (choreographer) {
    choreographer->Add<&AudioEngine::Update>(Choreographer::Stage::kRender);
  }
}

ResonanceAudioEngine::~ResonanceAudioEngine() {
  // Audio I/O must be stopped before destroying the AudioStreamRenderers.
  Stop();

  // The task queue holds AudioStreamRenderer instances that access resonance
  // during destruction, so they need to be cleared first.
  audio_thread_task_queue_.Clear();
}

void ResonanceAudioEngine::Start() {
  if (audio_running_) {
    return;
  }
  audio_running_ = true;

  if (!audio_stream_manager_) {
    audio_stream_manager_ = std::make_unique<AudioStreamManager>();
  }
  if (!audio_asset_manager_) {
    audio_asset_manager_ =
        std::make_unique<AudioAssetManager>(registry_, *speaker_profile_);
  }
  if (!resonance_) {
    resonance_.reset(vraudio::CreateResonanceAudioApi(
        vraudio::kNumStereoChannels, speaker_profile_->frames_per_buffer,
        speaker_profile_->sample_rate_hz));
    // Disable room effects by default.
    resonance_->EnableRoomEffects(false);
  }

  if (kCreateAudioThread) {
    fifo_.EnableBlockingSleepUntilMethods(true);
    audio_thread_ =
        std::thread(&ResonanceAudioEngine::AudioProcessingThread, this);
  }
  audio_stream_manager_->Start();

  auto device_manager = registry_->Get<DeviceManager>();
  device_manager->SetFillAudioBufferFn([=](Speaker::HwBuffer hw_buffer) {
    const size_t size = speaker_profile_->num_channels *
                        speaker_profile_->frames_per_buffer * sizeof(uint16);
    CHECK_EQ(hw_buffer.size(), size);
    auto data_ptr = reinterpret_cast<int16_t*>(hw_buffer.data());
    OnMoreData(data_ptr, speaker_profile_->num_channels,
               speaker_profile_->frames_per_buffer);
  });
}

void ResonanceAudioEngine::Stop() {
  if (!audio_running_) {
    return;
  }
  audio_running_ = false;

  auto device_manager = registry_->Get<DeviceManager>();
  device_manager->SetFillAudioBufferFn(nullptr);

  for (auto& sound : sounds_) {
    sound.second->Stop();
  }

  audio_stream_manager_->Stop();

  if (kCreateAudioThread && audio_thread_.joinable()) {
    fifo_.EnableBlockingSleepUntilMethods(false);
    audio_thread_.join();
  }
}

void ResonanceAudioEngine::Update() {
  auto device_manager = registry_->Get<DeviceManager>();
  const auto* speaker_profile = device_manager->Speaker(0).GetProfile();
  if (speaker_profile != nullptr) {
    if (speaker_profile_.has_value()) {
      // We can't tolerate a change here for now.
      CHECK_EQ(speaker_profile_->num_channels, speaker_profile->num_channels);
      CHECK_EQ(speaker_profile_->sample_rate_hz,
               speaker_profile->sample_rate_hz);
      CHECK_EQ(speaker_profile_->frames_per_buffer,
               speaker_profile->frames_per_buffer);
    } else {
      speaker_profile_.emplace();
      speaker_profile_->num_channels = speaker_profile->num_channels;
      speaker_profile_->sample_rate_hz = speaker_profile->sample_rate_hz;
      speaker_profile_->frames_per_buffer = speaker_profile->frames_per_buffer;
      CHECK_GT(speaker_profile_->num_channels, 0);
    }

    if (!audio_running_) {
      Start();
    }
  } else {
    if (audio_running_) {
      Stop();
    }
  }

  std::vector<SourceId> pending_delete;
  {
    std::lock_guard<std::mutex> lock(pending_delete_mutex_);
    pending_delete_.swap(pending_delete);
  }
  for (auto& source : pending_delete) {
    // Stopping the sound will call this->StopSound() which will erase it
    // from the sounds_ map. But, more importantly, it will invalidate the
    // Sound object itself, prevent it from being double-deleted.
    auto sound = sounds_.find(source)->second;
    sound->Stop();
  }
}

SoundPtr ResonanceAudioEngine::CreateSound(AudioAssetPtr asset,
                                           const SoundPlaybackParams& params) {
  auto playback_asset =
      audio_asset_manager_->GetAssetForPlayback(asset->GetId());
  if (!playback_asset) {
    return nullptr;
  }

  auto stream = std::make_unique<AudioAssetStream>(playback_asset,
                                                   speaker_profile_.value());
  if (!stream->Initialize()) {
    LOG(ERROR) << "AudioAssetStream failed to initialize.";
    return nullptr;
  }

  stream->EnableLooping(params.looping);
  // if (params.looping_crossfade_seconds >= 0.f) {
  //   stream->SetLoopCrossfadeDuration(params.loop_crossfade_seconds);
  // }

  auto mode = vraudio::RenderingMode::kBinauralHighQuality;
  auto renderer = std::make_shared<AudioStreamRenderer>(
      params.type, std::move(stream), resonance_.get(), mode);

  renderer->SetVolume(params.volume);

  const SourceId source_id = renderer->GetSourceId();
  CHECK(!sounds_.contains(source_id)) << "Duplicate source id: " << source_id;

  auto task = [=]() {
    const bool success =
        audio_stream_manager_->AddAudioStreamRenderer(renderer);
    CHECK(success) << "Renderer is already registered: " << source_id;
  };
  audio_thread_task_queue_.Post(task);

  auto sound = std::make_shared<ResonanceSound>(params.type, source_id, this);
  sounds_[source_id] = sound;
  return std::static_pointer_cast<Sound>(sound);
}

void ResonanceAudioEngine::PauseSound(SourceId source_id) {
  if (sounds_.contains(source_id)) {
    RunRendererTask(source_id,
                    [](AudioStreamRenderer& renderer) { renderer.Pause(); });
  }
}

void ResonanceAudioEngine::ResumeSound(SourceId source_id) {
  if (sounds_.contains(source_id)) {
    RunRendererTask(source_id,
                    [](AudioStreamRenderer& renderer) { renderer.Resume(); });
  }
}

void ResonanceAudioEngine::StopSound(SourceId source_id) {
  if (sounds_.contains(source_id)) {
    RunRendererTask(source_id,
                    [](AudioStreamRenderer& renderer) { renderer.Shutdown(); });
    {
      std::lock_guard<std::mutex> lock(pending_delete_mutex_);
      pending_delete_.push_back(source_id);
    }
  }
}

void ResonanceAudioEngine::SetSoundObjectPosition(SourceId source_id,
                                                  const vec3& position) {
  resonance_->SetSourcePosition(source_id, position.x, position.y, position.z);
}

void ResonanceAudioEngine::SetSoundObjectDistanceRolloffModel(
    SourceId source_id, Sound::DistanceRolloffModel rolloff, float min_distance,
    float max_distance) {
  resonance_->SetSourceDistanceModel(source_id, ToResonance(rolloff),
                                     min_distance, max_distance);
  if (rolloff == Sound::DistanceRolloffModel::NoRollof) {
    // No distance attenuation should be applied.
    resonance_->SetSourceDistanceAttenuation(source_id, 1.0f);
  }
}

void ResonanceAudioEngine::SetSoundfieldRotation(SourceId source_id,
                                                 const quat& rotation) {
  resonance_->SetSourceRotation(source_id, rotation.x, rotation.y, rotation.z,
                                rotation.w);
}

void ResonanceAudioEngine::SetSoundObjectDirectivity(SourceId source_id,
                                                     float alpha, float order) {
  resonance_->SetSoundObjectDirectivity(source_id, alpha, order);
}

void ResonanceAudioEngine::SetSoundObjectRotation(SourceId source_id,
                                                  const quat& rotation) {
  resonance_->SetSourceRotation(source_id, rotation.x, rotation.y, rotation.z,
                                rotation.w);
}

void ResonanceAudioEngine::SetSoundVolume(SourceId source_id, float volume) {
  if (sounds_.contains(source_id)) {
    RunRendererTask(source_id, [=](AudioStreamRenderer& renderer) {
      renderer.SetVolume(volume);
    });
  }
}

void ResonanceAudioEngine::SetGlobalVolume(float volume) {
  resonance_->SetMasterVolume(volume);
}

void ResonanceAudioEngine::SetListenerTransform(const vec3& position,
                                                const quat& rotation) {
  resonance_->SetHeadPosition(position.x, position.y, position.z);
  resonance_->SetHeadRotation(rotation.x, rotation.y, rotation.z, rotation.w);
}

void ResonanceAudioEngine::EnableRoom(const SoundRoom& room,
                                      const vec3& position,
                                      const quat& rotation) {
  auto room_properties = ToResonance(room, position, rotation);
  const auto reflection = vraudio::ComputeReflectionProperties(room_properties);
  const auto reverb = vraudio::ComputeReverbProperties(room_properties);

  resonance_->SetReflectionProperties(reflection);
  resonance_->SetReverbProperties(reverb);
  resonance_->EnableRoomEffects(true);
}

void ResonanceAudioEngine::DisableRoom() {
  resonance_->EnableRoomEffects(false);
}

void ResonanceAudioEngine::RunRendererTask(SourceId source_id,
                                           RendererTask fn) {
  auto task = [this, source_id, fn = std::move(fn)]() {
    auto renderer = audio_stream_manager_->GetAudioStreamRenderer(source_id);
    if (renderer != nullptr) {
      fn(*renderer);
    } else {
      std::lock_guard<std::mutex> lock(pending_delete_mutex_);
      pending_delete_.push_back(source_id);
    }
  };
  audio_thread_task_queue_.Post(task);
}

void ResonanceAudioEngine::OnMoreData(int16_t* buffer_ptr, size_t num_channels,
                                      size_t num_frames) {
  const size_t num_samples = num_channels * num_frames;
  if (num_frames != speaker_profile_->frames_per_buffer) {
    LOG(ERROR) << "OnMoreData called with unexpected frames per buffer size: "
               << num_frames;
    std::fill(buffer_ptr, buffer_ptr + num_samples, 0);
    return;
  }

  const vraudio::AudioBuffer* export_buffer = nullptr;
  std::unique_ptr<vraudio::AudioBuffer> audio_buffer_from_fifo;
  if (kCreateAudioThread) {
    // Wait till we have a buffer for sure.
    const std::chrono::steady_clock::duration wait_time =
        std::chrono::milliseconds(kAudioThreadFifoMaxWaitTimeMs);
    if (audio_running_ && fifo_.SleepUntilNumElementsInQueue(1, wait_time)) {
      audio_buffer_from_fifo = fifo_.PopFront();
      export_buffer = audio_buffer_from_fifo.get();
    }
  } else {
    export_buffer = BinauralProcessingOfSoundSources();
  }

  if (export_buffer == nullptr) {
    std::fill(buffer_ptr, buffer_ptr + num_samples, 0);
  } else if (num_channels == vraudio::kNumStereoChannels) {
    ExportToStereoBuffer(*export_buffer, buffer_ptr, num_channels, num_frames);
  } else if (num_channels == vraudio::kNumMonoChannels) {
    DownmixToMonoBuffer(*export_buffer, buffer_ptr, num_channels, num_frames);
  } else {
    UpmixToSurroundBuffer(*export_buffer, buffer_ptr, num_channels, num_frames);
  }
}

void ResonanceAudioEngine::ExportToStereoBuffer(
    const vraudio::AudioBuffer& input, int16_t* output, size_t num_channels,
    size_t num_frames) {
  FillExternalBuffer(input, output, num_frames, num_channels);
}

void ResonanceAudioEngine::DownmixToMonoBuffer(
    const vraudio::AudioBuffer& input, int16_t* output, size_t num_channels,
    size_t num_frames) {
  if (!mono_buffer_) {
    mono_buffer_ = std::make_unique<vraudio::AudioBuffer>(
        vraudio::kNumMonoChannels, speaker_profile_->frames_per_buffer);
  }
  ConvertMonoFromStereo(input, mono_buffer_.get());
  FillExternalBuffer(*mono_buffer_, output, num_frames, num_channels);
}

void ResonanceAudioEngine::UpmixToSurroundBuffer(
    const vraudio::AudioBuffer& input, int16_t* output, size_t num_channels,
    size_t num_frames) {
  const size_t num_samples = num_channels * num_frames;
  std::fill(output, output + num_samples, 0);

  for (size_t channel = 0; channel < vraudio::kNumStereoChannels; ++channel) {
    const auto& channel_view = input[channel];

    size_t interleaved_index = channel;
    for (size_t frame = 0; frame < num_frames; ++frame) {
      vraudio::ConvertSampleFromFloatFormat(channel_view[frame],
                                            &output[interleaved_index]);
      interleaved_index += num_channels;
    }
  }
}

const vraudio::AudioBuffer*
ResonanceAudioEngine::BinauralProcessingOfSoundSources() {
  audio_thread_task_queue_.Execute();

  std::vector<SourceId> disabled_renderers;
  audio_stream_manager_->Render(&disabled_renderers);
  {
    std::lock_guard<std::mutex> lock(pending_delete_mutex_);
    pending_delete_.insert(pending_delete_.end(), disabled_renderers.begin(),
                           disabled_renderers.end());
  }

  auto impl = static_cast<vraudio::ResonanceAudioApiImpl*>(resonance_.get());
  impl->ProcessNextBuffer();
  return impl->GetStereoOutputBuffer();
}

void ResonanceAudioEngine::AudioProcessingThread() {
  const std::chrono::steady_clock::duration wait_time =
      std::chrono::milliseconds(kAudioThreadFifoMaxWaitTimeMs);

  while (audio_running_) {
    if (fifo_.SleepUntilBelowSizeTarget(kAudioThreadFifoBufferSize,
                                        wait_time)) {
      const vraudio::AudioBuffer* new_audio_buffer =
          BinauralProcessingOfSoundSources();
      if (new_audio_buffer != nullptr) {
        // Since |new_audio_buffer| is owned and reused by the audio graph, a
        // copy is needed to push it to the output |fifo_|.
        auto audio_buffer_copy = std::make_unique<vraudio::AudioBuffer>();
        *audio_buffer_copy = *new_audio_buffer;
        fifo_.PushBack(std::move(audio_buffer_copy));
      } else {
        // This will generate a silence output.
        fifo_.PushBack(nullptr);
      }
    }
  }
}

// The following functions thunk the AudioEngine API to the ResonanceAudioEngine
// implementation.

inline ResonanceAudioEngine* Upcast(AudioEngine* ptr) {
  return static_cast<ResonanceAudioEngine*>(ptr);
}
void AudioEngine::Create(Registry* registry) {
  auto ptr = new ResonanceAudioEngine(registry);
  registry->Register(std::unique_ptr<AudioEngine>(ptr));
}
void AudioEngine::SetGlobalVolume(float volume) {
  Upcast(this)->SetGlobalVolume(volume);
}
void AudioEngine::EnableRoom(const SoundRoom& room, const vec3& position,
                             const quat& rotation) {
  Upcast(this)->EnableRoom(room, position, rotation);
}
void AudioEngine::DisableRoom() { Upcast(this)->DisableRoom(); }
void AudioEngine::SetListenerTransform(const vec3& position,
                                       const quat& rotation) {
  Upcast(this)->SetListenerTransform(position, rotation);
}
void AudioEngine::Update() { Upcast(this)->Update(); }
AudioAssetPtr AudioEngine::GetAudioAsset(HashValue key) {
  return Upcast(this)->GetAudioAssetManager()->FindAudioAsset(key);
}
AudioAssetPtr AudioEngine::LoadAudioAsset(std::string_view uri,
                                          StreamingPolicy policy) {
  return Upcast(this)->GetAudioAssetManager()->CreateAudioAsset(uri, policy);
}
void AudioEngine::UnloadAudioAsset(HashValue key) {
  Upcast(this)->GetAudioAssetManager()->UnloadAudioAsset(key);
}
SoundPtr AudioEngine::PrepareSound(AudioAssetPtr asset,
                                   const SoundPlaybackParams& params) {
  return Upcast(this)->CreateSound(asset, params);
}
SoundPtr AudioEngine::PlaySound(AudioAssetPtr asset,
                                const SoundPlaybackParams& params) {
  SoundPtr sound = PrepareSound(asset, params);
  sound->Resume();
  return sound;
}

static StaticRegistry Static_Register(AudioEngine::Create);

}  // namespace redux
