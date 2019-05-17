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

#include "lullaby/systems/audio/audio_system.h"

#include <utility>

#include "mathfu/constants.h"
#include "lullaby/generated/audio_environment_def_generated.h"
#include "lullaby/generated/audio_listener_def_generated.h"
#include "lullaby/generated/audio_response_def_generated.h"
#include "lullaby/generated/audio_source_def_generated.h"
#include "lullaby/events/audio_events.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/events/lifetime_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/dispatcher/event_wrapper.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/gvr/mathfu_gvr_conversions.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/android_context.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/util/math.h"
#include "lullaby/util/random_number_generator.h"
#include "lullaby/util/trace.h"

namespace lull {
namespace {

gvr::AudioMaterialName SelectMaterial(lull::AudioSurfaceMaterial name) {
  switch (name) {
    case lull::AudioSurfaceMaterial_Transparent:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_TRANSPARENT;

    case lull::AudioSurfaceMaterial_AcousticCeilingTiles:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_ACOUSTIC_CEILING_TILES;

    case lull::AudioSurfaceMaterial_BrickBare:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_BRICK_BARE;

    case lull::AudioSurfaceMaterial_BrickPainted:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_BRICK_PAINTED;

    case lull::AudioSurfaceMaterial_ConcreteBlockCoarse:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_CONCRETE_BLOCK_COARSE;

    case lull::AudioSurfaceMaterial_ConcreteBlockPainted:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_CONCRETE_BLOCK_PAINTED;

    case lull::AudioSurfaceMaterial_CurtainHeavy:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_CURTAIN_HEAVY;

    case lull::AudioSurfaceMaterial_FiberGlassInsulation:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_FIBER_GLASS_INSULATION;

    case lull::AudioSurfaceMaterial_GlassThin:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_GLASS_THIN;

    case lull::AudioSurfaceMaterial_GlassThick:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_GLASS_THICK;

    case lull::AudioSurfaceMaterial_Grass:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_GRASS;

    case lull::AudioSurfaceMaterial_LinoleumOnConcrete:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_LINOLEUM_ON_CONCRETE;

    case lull::AudioSurfaceMaterial_Marble:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_MARBLE;

    case lull::AudioSurfaceMaterial_Metal:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_METAL;

    case lull::AudioSurfaceMaterial_ParquetOnConcrete:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_PARQUET_ON_CONCRETE;

    case lull::AudioSurfaceMaterial_PlasterRough:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_PLASTER_ROUGH;

    case lull::AudioSurfaceMaterial_PlasterSmooth:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_PLASTER_SMOOTH;

    case lull::AudioSurfaceMaterial_PlywoodPanel:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_PLYWOOD_PANEL;

    case lull::AudioSurfaceMaterial_PolishedConcreteOrTile:
      return gvr::AudioMaterialName::
          GVR_AUDIO_MATERIAL_POLISHED_CONCRETE_OR_TILE;

    case lull::AudioSurfaceMaterial_Sheetrock:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_SHEET_ROCK;

    case lull::AudioSurfaceMaterial_WaterOrIceSurface:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_WATER_OR_ICE_SURFACE;

    case lull::AudioSurfaceMaterial_WoodCeiling:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_WOOD_CEILING;

    case lull::AudioSurfaceMaterial_WoodPanel:
      return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_WOOD_PANEL;

    default:
      LOG(DFATAL) << "unknown Surface Material";
  }
  return gvr::AudioMaterialName::GVR_AUDIO_MATERIAL_TRANSPARENT;
}

}  // namespace

// GVR expects an orthogonal head-from-world matrix for the head pose.
gvr::Mat4f ConvertToGvrHeadPoseMatrix(
    const mathfu::mat4& world_from_entity_mat) {
  // Decompose the matrix, force uniform scale, and then recompose the matrix to
  // guarantee orthogonality.
  Sqt sqt = CalculateSqtFromMatrix(world_from_entity_mat);
  sqt.scale = mathfu::kOnes3f;
  const mathfu::mat4 unscaled_world_mat = CalculateTransformMatrix(sqt);

  // Invert the world-from-entity matrix to get the entity-from-world matrix.
  // Because the Entity is the "head", this is the final matrix.
  return GvrMatFromMathfu(unscaled_world_mat.Inverse());
}

const HashValue kAudioEnvironmentDef = ConstHash("AudioEnvironmentDef");
const HashValue kAudioListenerDef = ConstHash("AudioListenerDef");
const HashValue kAudioResponseDef = ConstHash("AudioResponseDef");
const HashValue kAudioSourceDef = ConstHash("AudioSourceDef");
const gvr::AudioRenderingMode kQuality =
    gvr::AudioRenderingMode::GVR_AUDIO_RENDERING_BINAURAL_HIGH_QUALITY;

AudioSystem::AudioSystem(Registry* registry)
    : AudioSystem(registry, MakeUnique<gvr::AudioApi>()) {}

AudioSystem::AudioSystem(Registry* registry, std::unique_ptr<gvr::AudioApi> api)
    : System(registry),
      sources_(16),
      environments_(16),
      audio_(std::move(api)),
      listener_(kNullEntity),
      current_environment_(kNullEntity),
      audio_running_(false),
      transform_flag_(TransformSystem::kInvalidFlag),
      master_volume_(1.0),
      muted_(false) {
  if (audio_) {
    // Only Init() the gvr::AudioApi if it's backing C instance doesn't exist
    // else an already-in-use instance might be destroyed.
    if (!audio_->cobj()) {
      InitAudio();

      if (!audio_running_) {
        LOG(ERROR) << "Starting audio system failed.";
      }
    } else {
      // Assume the audio instance is initialized and Resume() it to ensure it
      // is actually running.
      audio_->Resume();
      audio_running_ = true;
    }
    audio_->EnableStereoSpeakerMode(true);
  }
  asset_manager_.reset(new SoundAssetManager(registry, audio_));

  RegisterDef(this, kAudioEnvironmentDef);
  RegisterDef(this, kAudioListenerDef);
  RegisterDef(this, kAudioResponseDef);
  RegisterDef(this, kAudioSourceDef);

  RegisterDependency<DispatcherSystem>(this);

  auto* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->Connect(this, [this](const OnPauseThreadUnsafeEvent& event) {
    std::lock_guard<std::mutex> lock(pause_mutex_);
    if (audio_) {
      audio_->Pause();
      audio_running_ = false;
    }
  });
  dispatcher->Connect(this, [this](const OnResumeEvent& event) {
    std::lock_guard<std::mutex> lock(pause_mutex_);
    if (audio_) {
      // Resuming acts like setting a new AudioListener. Re-set the master
      // volume as if the current listener was just created, but respect any
      // previous mute state that may have been set.
      master_volume_ = listener_.initial_volume;
      if (!muted_) {
        audio_->SetMasterVolume(listener_.initial_volume);
      }
      audio_->Resume();
      audio_running_ = true;
    }
  });
  dispatcher->Connect(this, [this](const DisableAudioEnvironmentEvent& event) {
    SetEnvironment(kNullEntity);
  });
  dispatcher->Connect(this, [this](const OnEnabledEvent& event) {
    OnEntityEnabled(event.target);
  });
  dispatcher->Connect(this, [this](const OnDisabledEvent& event) {
    OnEntityDisabled(event.target);
  });

  auto* binder = registry->Get<lull::FunctionBinder>();
  if (binder) {
    binder->RegisterMethod("lull.Audio.Play", &AudioSystem::Play);
    binder->RegisterMethod("lull.Audio.Stop", &AudioSystem::Stop);
    binder->RegisterMethod("lull.Audio.StopAll", &AudioSystem::StopAll);
    binder->RegisterMethod("lull.Audio.SetMute", &AudioSystem::SetMute);

    binder->RegisterFunction(
        "lull.Audio.Pause",
        [this](Entity e, HashValue sound) { Pause(e, sound); });
    binder->RegisterFunction(
        "lull.Audio.Resume",
        [this](Entity e, HashValue sound) { Resume(e, sound); });

    binder->RegisterFunction(
        "lull.Audio.SetVolume",
        [this](Entity e, float volume, HashValue sound) {
          SetVolume(e, volume, sound);
        });

    binder->RegisterFunction(
        "lull.Audio.LoadSound",
        [this](const std::string& filename, AudioLoadType type) {
          LoadSound(filename, type);
        });
    binder->RegisterMethod("lull.Audio.UnloadSound", &AudioSystem::UnloadSound);

    // Registering functions to access the enum values of AudioSourceType.
    binder->RegisterFunction("lull.Audio.SourceType.Stereo", []() {
      return AudioSourceType::AudioSourceType_Stereo;
    });
    binder->RegisterFunction("lull.Audio.SourceType.SoundObject", []() {
      return AudioSourceType::AudioSourceType_SoundObject;
    });
    binder->RegisterFunction("lull.Audio.SourceType.Soundfield", []() {
      return AudioSourceType::AudioSourceType_Soundfield;
    });

    // Registering functions to access the enum values of AudioPlaybackType.
    // Note that there is no function for playback type 'External' as it is only
    // allowed to be called from within the audio system.
    binder->RegisterFunction("lull.Audio.PlaybackType.PlayIfReady", []() {
      return AudioPlaybackType::AudioPlaybackType_PlayIfReady;
    });
    binder->RegisterFunction("lull.Audio.PlaybackType.PlayWhenReady", []() {
      return AudioPlaybackType::AudioPlaybackType_PlayWhenReady;
    });

    // Registering functions to access the enum values of
    // gvr::AudioRolloffMethod.
    binder->RegisterFunction("lull.Audio.RolloffMethod.Logarithmic", []() {
      return gvr::AudioRolloffMethod::GVR_AUDIO_ROLLOFF_LOGARITHMIC;
    });
    binder->RegisterFunction("lull.Audio.RolloffMethod.Linear", []() {
      return gvr::AudioRolloffMethod::GVR_AUDIO_ROLLOFF_LINEAR;
    });
    binder->RegisterFunction("lull.Audio.RolloffMethod.None", []() {
      return gvr::AudioRolloffMethod::GVR_AUDIO_ROLLOFF_NONE;
    });

    // Registering functions to access the enum values of AudioLoadType.
    binder->RegisterFunction("lull.Audio.LoadType.Preload", []() {
      return AudioLoadType::AudioLoadType_Preload;
    });
    binder->RegisterFunction("lull.Audio.LoadType.Stream", []() {
      return AudioLoadType::AudioLoadType_Stream;
    });
  }

  // Guarantee that the RNG exists, else AudioResponseDefs may not work
  // correctly.
  registry_->Create<RandomNumberGenerator>();
}

void AudioSystem::InitAudio() {
#ifdef __ANDROID__
  auto* android_context = registry_->Get<AndroidContext>();
  if (android_context) {
    audio_running_ =
        audio_->Init(android_context->GetJniEnv(),
                     android_context->GetApplicationContext().Get(),
                     android_context->GetClassLoader().Get(), kQuality);
  } else {
    LOG(DFATAL) << "GVR Audio Init() failed due to missing AndroidContext.";
  }
#else
  audio_running_ = audio_->Init(kQuality);
#endif  // #ifdef __ANDROID__
}


AudioSystem::~AudioSystem() {
  // Explicitly destroy the asset manager first to ensure that pending loads are
  // flushed before the audio instance is destroyed.
  asset_manager_.reset();
  // This destructor will stop GVR Audio processing.
  audio_.reset();
  auto* dispatcher = registry_->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->DisconnectAll(this);
  }
  auto* transform_system = registry_->Get<TransformSystem>();
  if (transform_system && transform_flag_ != TransformSystem::kInvalidFlag) {
    transform_system->ReleaseFlag(transform_flag_);
  }
  auto* binder = registry_->Get<lull::FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.Audio.Play");
    binder->UnregisterFunction("lull.Audio.Stop");
    binder->UnregisterFunction("lull.Audio.StopAll");
    binder->UnregisterFunction("lull.Audio.SetMute");
    binder->UnregisterFunction("lull.Audio.Pause");
    binder->UnregisterFunction("lull.Audio.Resume");
    binder->UnregisterFunction("lull.Audio.SetVolume");
    binder->UnregisterFunction("lull.Audio.LoadSound");
    binder->UnregisterFunction("lull.Audio.UnloadSound");
    binder->UnregisterFunction("lull.Audio.SourceType.Stereo");
    binder->UnregisterFunction("lull.Audio.SourceType.SoundObject");
    binder->UnregisterFunction("lull.Audio.SourceType.Soundfield");
    binder->UnregisterFunction("lull.Audio.PlaybackType.PlayIfReady");
    binder->UnregisterFunction("lull.Audio.PlaybackType.PlayWhenReady");
    binder->UnregisterFunction("lull.Audio.RolloffMethod.Logarithmic");
    binder->UnregisterFunction("lull.Audio.RolloffMethod.Linear");
    binder->UnregisterFunction("lull.Audio.RolloffMethod.None");
    binder->UnregisterFunction("lull.Audio.LoadType.Preload");
    binder->UnregisterFunction("lull.Audio.LoadType.Stream");
  }
}

void AudioSystem::Initialize() {
  auto* transform_system = registry_->Get<TransformSystem>();
  transform_flag_ = transform_system->RequestFlag();
}

void AudioSystem::Create(Entity e, HashValue type, const Def* def) {
  if (!def) {
    LOG(DFATAL) << "Must provide valid Def!";
    return;
  }

  if (type == kAudioSourceDef) {
    const auto* data = ConvertDef<AudioSourceDef>(def);
    CreateSource(e, data);
  } else if (type == kAudioListenerDef) {
    const auto* data = ConvertDef<AudioListenerDef>(def);
    CreateListener(e, data);
  } else if (type == kAudioResponseDef) {
    const auto* data = ConvertDef<AudioResponseDef>(def);
    CreateResponse(e, data);
  } else if (type == kAudioEnvironmentDef) {
    const auto* data = ConvertDef<AudioEnvironmentDef>(def);
    CreateEnvironment(e, data);
  } else {
    LOG(DFATAL) << "Unsupported ComponentDef type: " << type;
  }
}

void AudioSystem::Destroy(Entity e) {
  StopAll(e);
  if (listener_.GetEntity() == e) {
    listener_ = AudioListener(kNullEntity);
  }
  if (e == current_environment_) {
    SetEnvironment(kNullEntity);
  }
  environments_.Destroy(e);
}

void AudioSystem::CreateEnvironment(Entity e, const AudioEnvironmentDef* data) {
  auto* model = environments_.Emplace(e);
  MathfuVec3FromFbVec3(data->room_dimensions(), &model->room_dimensions);
  model->reverb_brightness_modifier = data->reverb_brightness_modifier();
  model->reverb_gain = data->reverb_gain();
  model->reverb_time = data->reverb_time();
  model->surface_material_wall = SelectMaterial(data->surface_material_wall());
  model->surface_material_ceiling =
      SelectMaterial(data->surface_material_ceiling());
  model->surface_material_floor =
      SelectMaterial(data->surface_material_floor());
  auto response = [this, e](const EventWrapper&) { SetEnvironment(e); };

  ConnectEventDefs(registry_, e, data->set_environment_events(), response);

  if (data->enable_on_create()) {
    SetEnvironment(e);
  }
}

void AudioSystem::CreateSource(Entity e, const AudioSourceDef* data) {
  if (!data->sound()) {
    LOG(DFATAL) << "Must specify sound name!";
    return;
  }

  auto* name = data->sound();
  if (name == nullptr) {
    LOG(DFATAL) << "AudioSource specified with no sound was ignored.";
    return;
  }

  LoadSound(name->c_str(), data->load_type(), e);

  const HashValue sound_hash = Hash(name->c_str());
  PlaySoundParameters params;
  params.playback_type = data->playback_type();
  params.volume = data->volume();
  params.loop = data->loop();
  params.source_type = data->source_type();
  params.spatial_directivity_alpha = data->spatial_directivity_alpha();
  params.spatial_directivity_order = data->spatial_directivity_order();

  Play(e, sound_hash, params);
}

void AudioSystem::CreateListener(Entity e, const AudioListenerDef* data) {
  if (listener_.GetEntity() != kNullEntity) {
    LOG(WARNING) << "Audio Listener already existed when "
                    "AudioSystem.CreateListener() was called.  Reassigning "
                    "listener from "
                 << listener_.GetEntity() << " to " << e;
  }
  listener_ = AudioListener(e);
  listener_.initial_volume = data->initial_volume();
  master_volume_ = data->initial_volume();
  if (audio_) {
    audio_->SetMasterVolume(master_volume_);
  }
}

void AudioSystem::CreateResponse(Entity entity, const AudioResponseDef* data) {
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  if (!dispatcher_system) {
    // Early out so we don't load the sound file.
    return;
  }

  const AudioLoadType load_type = data->load_type();

  PlaySoundParameters params;
  params.playback_type = data->playback_type();
  params.volume = data->volume();
  params.source_type = data->source_type();
  params.spatial_directivity_alpha = data->spatial_directivity_alpha();
  params.spatial_directivity_order = data->spatial_directivity_order();

  if (data->sound()) {
    const auto* name = data->sound();
    LoadSound(name->c_str(), load_type, entity);
    const HashValue sound_hash = Hash(name->c_str());

    auto response = [this, entity, sound_hash, params](const EventWrapper&) {
      Play(entity, sound_hash, params);
    };

    ConnectEventDefs(registry_, entity, data->inputs(), response);
  } else if (data->random_sounds() && data->random_sounds()->size() > 0) {
    const auto* random_names = data->random_sounds();
    for (auto* name : *random_names) {
      LoadSound(name->c_str(), load_type, entity);
    }

    auto response = [this, entity, random_names, params](const EventWrapper&) {
      const int idx = registry_->Get<RandomNumberGenerator>()->GenerateUniform(
          0, static_cast<int>(random_names->size()) - 1);
      const HashValue sound_hash = Hash((*random_names)[idx]->c_str());
      Play(entity, sound_hash, params);
    };

    ConnectEventDefs(registry_, entity, data->inputs(), response);
  } else {
    LOG(DFATAL) << "AudioResponse specified with no sound(s) was ignored.";
  }
}

void AudioSystem::Play(Entity e, HashValue sound_hash,
                       const PlaySoundParameters& params) {
  // AudioPlaybackType::Stream is equivalent to PlayWhenReady.
  AudioPlaybackType playback_type = params.playback_type;
  if (playback_type == AudioPlaybackType::AudioPlaybackType_StreamDeprecated) {
    LOG(DFATAL) << "AudioPlaybackType::Stream is deprecated. Use "
                   "AudioLoadType::Stream and "
                   "AudioPlaybackType::PlayWhenReady for identical behavior.";
    return;
  }

  if (playback_type == AudioPlaybackType::AudioPlaybackType_External) {
    LOG(DFATAL) << "AudioPlaybackType::External is reserved exclusively for "
                   "Track(), and cannot be attached to normal sources.";
    return;
  }

  if (!audio_) {
    return;
  }

  auto asset = asset_manager_->GetSoundAsset(sound_hash);
  if (!asset) {
    LOG(WARNING) << "Sound asset is either unloaded or released.";
    return;
  }

  // If the asset failed to load, or if it was set to only play if ready, skip
  // playing the sound.
  SoundAsset::LoadStatus status = asset->GetLoadStatus();
  if ((playback_type == AudioPlaybackType::AudioPlaybackType_PlayIfReady &&
       status == SoundAsset::LoadStatus::kUnloaded) ||
      status == SoundAsset::LoadStatus::kFailed) {
    return;
  }

  // Skip playing sounds altogether on disabled Entities.
  auto* transform_system = registry_->Get<TransformSystem>();
  if (!transform_system->IsEnabled(e)) {
    return;
  }
  Sqt sqt =
      CalculateSqtFromMatrix(transform_system->GetWorldFromEntityMatrix(e));

  auto* source = sources_.Get(e);
  if (source == nullptr) {
    source = sources_.Emplace(e);
    // Only set the SQT on newly-created sources. Doing so on an existing source
    // might result in already-playing sounds not getting transform updates.
    source->sqt = sqt;
  } else {
    // Stop and remove the sound as we are being asked to restart it.
    auto iter = source->sounds.find(sound_hash);
    if (iter != source->sounds.end()) {
      LOG(WARNING) << "Restarting sound: " << asset->GetFilename();
      audio_->StopSound(iter->second.id);
      source->sounds.erase(iter);
    }
  }

  transform_system->SetFlag(source->GetEntity(), transform_flag_);

  auto& sound = source->sounds[sound_hash];
  sound.id = kInvalidSourceId;
  sound.asset_handle = asset;
  sound.params = params;
  // Use the sanitized playback type.
  sound.params.playback_type = playback_type;

  // Volume > 1.0 is allowed and used for gain by GVR Audio.
  if (params.volume < 0.f) {
    LOG(WARNING) << "Volume must be >= 0 for audio, clamped to 0.";
    sound.params.volume = 0.f;
  }

  // Only play-when-ready sounds that are still unloaded should be skipped at
  // this point.
const bool ready =
    playback_type != AudioPlaybackType::AudioPlaybackType_PlayWhenReady ||
    status != SoundAsset::LoadStatus::kUnloaded;

  if (ready) {
    PlaySound(&sound, sqt, asset);
  }
}

void AudioSystem::Stop(Entity e, HashValue key) {
  if (!audio_) {
    return;
  }

  auto* source = sources_.Get(e);
  if (!source) {
    return;
  }

  auto iter = source->sounds.find(key);
  if (iter != source->sounds.end()) {
    if (iter->second.params.playback_type ==
        AudioPlaybackType::AudioPlaybackType_External) {
      LOG(WARNING) << "Attempted to Stop() an external audio source.";
      return;
    }
    audio_->StopSound(iter->second.id);
    source->sounds.erase(iter);
  } else {
    LOG(WARNING) << "Failed to find the specified sound to Stop().";
    return;
  }

  TryDestroySource(e);
}

void AudioSystem::PauseSound(Sound* sound) {
  if (!sound->paused) {
    audio_->PauseSound(sound->id);
    sound->paused = true;
  }
}

void AudioSystem::Pause(Entity e, HashValue sound) {
  auto* source = sources_.Get(e);
  if (source == nullptr || !source->enabled) {
    return;
  }

  // If a sound was specified, pause that sound. Otherwise, pause all sounds.
  if (sound != 0) {
    auto iter = source->sounds.find(sound);
    if (iter != source->sounds.end()) {
      PauseSound(&iter->second);
    } else {
      LOG(WARNING) << "Failed to find the specified sound to pause.";
    }
  } else {
    for (auto& iter : source->sounds) {
      PauseSound(&iter.second);
    }
  }
}

void AudioSystem::ResumeSound(Sound* sound) {
  if (sound->paused) {
    audio_->ResumeSound(sound->id);
    sound->paused = false;
  }
}

void AudioSystem::Resume(Entity e, HashValue sound) {
  auto* source = sources_.Get(e);
  if (source == nullptr || !source->enabled) {
    return;
  }

  // If a sound was specified, resume that sound. Otherwise, resume all sounds.
  if (sound != 0) {
    auto iter = source->sounds.find(sound);
    if (iter != source->sounds.end()) {
      ResumeSound(&iter->second);
    } else {
      LOG(WARNING) << "Failed to find the specified sound to resume.";
    }
  } else {
    for (auto& iter : source->sounds) {
      ResumeSound(&iter.second);
    }
  }
}

void AudioSystem::Track(Entity entity, gvr::AudioSourceId id, HashValue key,
                        float volume, AudioSourceType source_type) {
  if (!audio_ || !audio_->IsSourceIdValid(id)) {
    LOG(DFATAL) << "Given id is not associated with this GVR Audio instance.";
    return;
  }

  // Volume > 1.0 is allowed and used for gain by GVR Audio.
  if (volume < 0.f) {
    LOG(WARNING) << "Volume must be >= 0 for audio, clamped to 0.";
    volume = 0.f;
  }

  auto* transform_system = registry_->Get<TransformSystem>();
  Sqt sqt = CalculateSqtFromMatrix(
      transform_system->GetWorldFromEntityMatrix(entity));

  auto* source = sources_.Get(entity);
  if (source == nullptr) {
    source = sources_.Emplace(entity);
    // Only set the SQT on newly-created sources. Doing so on existing source
    // might result in already-playing sounds not getting transform updates.
    source->sqt = sqt;
  } else {
    auto iter = source->sounds.find(key);
    if (iter != source->sounds.end()) {
      LOG(WARNING) << "Key already in use. Source will not be tracked.";
      return;
    }
  }

  transform_system->SetFlag(entity, transform_flag_);

  auto& sound = source->sounds[key];

  // "asset_handle" is completely unused.
  sound.id = id;
  sound.params.volume = volume;
  // "loop" is completely unused.
  sound.params.loop = true;
  sound.params.source_type = source_type;
  sound.params.playback_type = AudioPlaybackType::AudioPlaybackType_External;

  audio_->SetSoundVolume(id, volume);
  UpdateSoundTransform(&sound, sqt);
}

void AudioSystem::Untrack(Entity e, HashValue key) {
  auto* source = sources_.Get(e);
  if (!source) {
    return;
  }

  auto iter = source->sounds.find(key);
  if (iter != source->sounds.end()) {
    if (iter->second.params.playback_type !=
        AudioPlaybackType::AudioPlaybackType_External) {
      LOG(WARNING) << "Attempted to Untrack() a non-external audio source.";
      return;
    }
    source->sounds.erase(iter);
  } else {
    LOG(WARNING) << "Failed to find the specified sound to Untrack().";
    return;
  }

  TryDestroySource(e);
}

void AudioSystem::StopAll(Entity e) {
  auto* source = sources_.Get(e);
  if (!source) {
    return;
  }

  if (audio_) {
    for (auto& iter : source->sounds) {
      if (iter.second.params.playback_type !=
          AudioPlaybackType::AudioPlaybackType_External) {
        audio_->StopSound(iter.second.id);
      }
    }
  }

  source->sounds.clear();
  TryDestroySource(e);
}

void AudioSystem::TryDestroySource(Entity entity) {
  auto* source = sources_.Get(entity);
  if (source == nullptr) {
    return;
  }

  if (source->sounds.empty()) {
    sources_.Destroy(entity);
    auto* transform_system = registry_->Get<TransformSystem>();
    transform_system->ClearFlag(entity, transform_flag_);
  }
}

void AudioSystem::UpdateSource(AudioSource* source,
                               const mathfu::mat4& world_from_entity) {
  // Early-exit on Entities that are enabled in the TransformSystem but haven't
  // had their OnEnabledEvent dispatched yet.
  if (!source->enabled) {
    return;
  }

  const bool sqt_updated = UpdateSourceSqt(source, world_from_entity);
  for (auto iter = source->sounds.begin(); iter != source->sounds.end();) {
    Sound& sound = iter->second;
    // Skip paused sounds or they'll be deleted.
    if (sound.paused) {
      ++iter;
      continue;
    }
    const SourceId source_id = sound.id;
    SoundAsset::LoadStatus status = SoundAsset::LoadStatus::kUnloaded;
    SoundAssetPtr asset = nullptr;

    // External playback sources will have null assets, but are considered
    // loaded.
    if (sound.params.playback_type ==
        AudioPlaybackType::AudioPlaybackType_External) {
      status = SoundAsset::LoadStatus::kLoaded;
    } else {
      // Non-looping sounds with unloaded assets can be forgotten and will be
      // cleaned up by GVR Audio. Looping sounds must be held onto else they
      // cannot be stopped.
      asset = sound.asset_handle.lock();
      if (asset) {
        status = asset->GetLoadStatus();
      } else if (!sound.params.loop) {
        status = SoundAsset::LoadStatus::kFailed;
      }
    }

    // 1. Clear out any sounds that failed to load or stream.
    // 2. If a play-when-ready sound is unloaded, skip it.
    // 3. If a sound hasn't been assigned an id yet, play it. If it fails to
    //    play for some reason, it will be cleaned up next Update().
    // 4. If a sound is currently playing, update its transform.
    // 5. If none of the above is true, this sound is done playing and should
    //    be cleaned up.
    if (status == SoundAsset::LoadStatus::kFailed) {
      iter = source->sounds.erase(iter);
    } else if (status == SoundAsset::LoadStatus::kUnloaded) {
      ++iter;
    } else if (source_id == kInvalidSourceId) {
      PlaySound(&sound, source->sqt, asset);
      ++iter;
    } else if (audio_->IsSoundPlaying(source_id)) {
      if (sqt_updated) {
        UpdateSoundTransform(&sound, source->sqt);
      }
      ++iter;
    } else {
      iter = source->sounds.erase(iter);
    }
  }

  // This call may invalidate |source|.
  TryDestroySource(source->GetEntity());
}

bool AudioSystem::UpdateSourceSqt(AudioSource* source, const mathfu::mat4& m) {
  static const float kUpdateThreshold = 0.001f;
  const Sqt new_sqt = CalculateSqtFromMatrix(m);

  const float dist_sq =
      (new_sqt.translation - source->sqt.translation).LengthSquared();
  if (dist_sq > kUpdateThreshold) {
    source->sqt = new_sqt;
    return true;
  }

  if (!AreNearlyEqual(source->sqt.rotation, new_sqt.rotation)) {
    source->sqt = new_sqt;
    return true;
  }
  return false;
}

void AudioSystem::Update() {
  // An OnPauseThreadUnsafeEvent may turn off audio playback while updating,
  // which can incorrectly label some sounds as not running anymore.
  std::lock_guard<std::mutex> lock(pause_mutex_);

  LULLABY_CPU_TRACE_CALL();
  if (!audio_ || !audio_running_) {
    return;
  }

  asset_manager_->ProcessTasks();

  auto* transform_system = registry_->Get<TransformSystem>();
  const auto* world_mat =
      transform_system->GetWorldFromEntityMatrix(listener_.GetEntity());
  if (world_mat) {
    audio_->SetHeadPose(ConvertToGvrHeadPoseMatrix(*world_mat));
  }

  transform_system->ForEach(
      transform_flag_,
      [this](Entity e, const mathfu::mat4& world_from_entity_mat,
             const Aabb& box) {
        auto* source = sources_.Get(e);
        UpdateSource(source, world_from_entity_mat);
      });

  audio_->Update();
}

void AudioSystem::SetMute(bool muted) {
  muted_ = muted;
  if (audio_) {
    audio_->SetMasterVolume(muted ? 0.f : master_volume_);
  }
}

bool AudioSystem::IsMute() const { return muted_; }

void AudioSystem::SetMasterVolume(float volume) {
  master_volume_ = volume;
  if (audio_) {
    audio_->SetMasterVolume(volume);
    muted_ = false;
  }
}

float AudioSystem::GetMasterVolume() const { return master_volume_; }

bool AudioSystem::HasSound(Entity e) const {
  auto* source = sources_.Get(e);
  if (source == nullptr) {
    return false;
  }
  return !source->sounds.empty();
}

std::vector<string_view> AudioSystem::GetSounds(Entity e) const {
  std::vector<string_view> output;
  auto* source = sources_.Get(e);
  if (source != nullptr) {
    for (auto& iter : source->sounds) {
      auto asset = iter.second.asset_handle.lock();
      if (asset) {
        output.emplace_back(asset->GetFilename());
      }
    }
  }
  return output;
}

void AudioSystem::SetVolume(Entity e, float volume, HashValue sound) {
  // Volume > 1.0 is allowed and used for gain by GVR Audio.
  if (volume < 0.f) {
    volume = 0.f;
  }

  if (!audio_) {
    return;
  }

  if (e == listener_.GetEntity()) {
    SetMasterVolume(volume);
    return;
  }

  auto* source = sources_.Get(e);
  if (source == nullptr) {
    // This may happen if a source is not fully loaded after it is created.
    return;
  }

  // If a sound was specified, set the volume of that sound. Otherwise, set the
  // volume of all sounds.
  if (sound != 0) {
    auto iter = source->sounds.find(sound);
    if (iter != source->sounds.end()) {
      auto& sound = iter->second;
      audio_->SetSoundVolume(sound.id, volume);
      sound.params.volume = volume;
    } else {
      LOG(WARNING) << "Failed to find the specified sound to change the "
                      "volume.";
    }
  } else {
    for (auto& iter : source->sounds) {
      auto& sound = iter.second;
      audio_->SetSoundVolume(sound.id, volume);
      sound.params.volume = volume;
    }
  }
}

float AudioSystem::GetVolume(Entity e, HashValue sound) const {
  if (e == listener_.GetEntity()) {
    return master_volume_;
  }

  auto* source = sources_.Get(e);
  if (source == nullptr) {
    // This may happen if a source is not fully loaded after it is created. A
    // non-loaded sound is not playing, and therefore has 0 volume.
    return 0.f;
  }

  // If a sound was specified, get the volume of that sound. Otherwise,
  // arbitrarily return the volume of the first sound on this source to support
  // volume animations.
  if (sound != 0) {
    auto iter = source->sounds.find(sound);
    if (iter != source->sounds.end()) {
      return iter->second.params.volume;
    } else {
      LOG(WARNING) << "Failed to find the specified sound to retrieve the "
                      "volume.";
    }
  } else {
    if (!source->sounds.empty()) {
      auto iter = source->sounds.begin();
      return iter->second.params.volume;
    }
  }

  // Return 0 volume for sounds that aren't properly set up.
  return 0.f;
}

Entity AudioSystem::GetCurrentListener() const { return listener_.GetEntity(); }

void AudioSystem::SetEnvironment(Entity e) {
  if (audio_ == nullptr || e == current_environment_) {
    return;
  }

  if (e == kNullEntity) {
    current_environment_ = e;
    audio_->EnableRoom(false);
    return;
  }

  auto* model = environments_.Get(e);
  if (!model) {
    LOG(DFATAL) << "No Audio Environment component associated with Entity.";
    return;
  }

  current_environment_ = e;
  audio_->SetRoomProperties(
      model->room_dimensions.x, model->room_dimensions.y,
      model->room_dimensions.z, model->surface_material_wall,
      model->surface_material_ceiling, model->surface_material_floor);
  audio_->SetRoomReverbAdjustments(model->reverb_gain, model->reverb_time,
                                   model->reverb_brightness_modifier);
  audio_->EnableRoom(true);
}

void AudioSystem::SetSoundObjectDirectivity(Entity entity, HashValue key,
                                            float alpha, float order) {
  if (!audio_) {
    return;
  }

  auto* source = sources_.Get(entity);
  if (source == nullptr) {
    // This may happen if a source is not fully loaded after it is created.
    return;
  }

  auto iter = source->sounds.find(key);
  if (iter != source->sounds.end()) {
    auto& sound = iter->second;
    if (sound.params.source_type !=
        AudioSourceType::AudioSourceType_SoundObject) {
      LOG(WARNING) << "Directivity can only be set on sound objects.";
      return;
    }
    sound.params.spatial_directivity_alpha = alpha;
    sound.params.spatial_directivity_order = order;
    audio_->SetSoundObjectDirectivity(sound.id, alpha, order);
    UpdateSoundTransform(&sound, source->sqt);
  } else {
    LOG(WARNING) << "Failed to find the specified sound to set directivity "
                    "constants.";
  }
}

bool AudioSystem::IsSpatialDirectivityEnabled(Sound* sound) const {
  return sound->params.source_type ==
             AudioSourceType::AudioSourceType_SoundObject &&
         sound->params.spatial_directivity_alpha >= 0 &&
         sound->params.spatial_directivity_alpha <= 1 &&
         sound->params.spatial_directivity_order >= 1;
}

void AudioSystem::SetSoundObjectDistanceRolloffMethod(
    Entity entity, HashValue key, gvr::AudioRolloffMethod method,
    float min_distance, float max_distance) {
  if (!audio_) {
    return;
  }

  if (min_distance > max_distance || min_distance < 0) {
    LOG(WARNING) << "Maximum distance must be greater than minimum distance, "
                    "and both must be >= 0. Rolloff model will not be set.";
    return;
  }

  auto* source = sources_.Get(entity);
  if (source == nullptr) {
    // This may happen if a source is not fully loaded after it is created.
    LOG(WARNING) << "Could not find the specified sound.";
    return;
  }

  auto iter = source->sounds.find(key);
  if (iter != source->sounds.end()) {
    auto& sound = iter->second;
    if (sound.params.source_type !=
        AudioSourceType::AudioSourceType_SoundObject) {
      LOG(WARNING) << "Rolloff can only be set on sound objects.";
      return;
    }
    sound.params.spatial_rolloff_method = method;
    sound.params.spatial_rolloff_min_distance = min_distance;
    sound.params.spatial_rolloff_max_distance = max_distance;
    audio_->SetSoundObjectDistanceRolloffModel(sound.id, method, min_distance,
                                               max_distance);
  } else {
    LOG(WARNING) << "Failed to find the specified sound to set the rolloff "
                    "method.";
  }
}

bool AudioSystem::IsDistanceRolloffMethodEnabled(Sound* sound) const {
  return sound->params.source_type ==
             AudioSourceType::AudioSourceType_SoundObject &&
         sound->params.spatial_rolloff_min_distance >= 0.0f &&
         sound->params.spatial_rolloff_max_distance > 0.0f;
}

void AudioSystem::PlaySound(Sound* sound, const Sqt& sqt,
                            const SoundAssetPtr& asset) {
  if (!audio_) {
    sound->id = kInvalidSourceId;
    return;
  }

  if (sound->id != kInvalidSourceId) {
    return;
  }

  SourceId new_id = kInvalidSourceId;
  switch (sound->params.source_type) {
    case AudioSourceType::AudioSourceType_Soundfield:
      new_id = audio_->CreateSoundfield(asset->GetFilename());
      break;
    case AudioSourceType::AudioSourceType_SoundObject:
      new_id = audio_->CreateSoundObject(asset->GetFilename());
      break;
    default:
      new_id = audio_->CreateStereoSound(asset->GetFilename());
      break;
  }

  sound->id = new_id;
  if (new_id != kInvalidSourceId) {
    if (IsSpatialDirectivityEnabled(sound)) {
      audio_->SetSoundObjectDirectivity(
          sound->id, sound->params.spatial_directivity_alpha,
          sound->params.spatial_directivity_order);
    }
    if (IsDistanceRolloffMethodEnabled(sound)) {
      audio_->SetSoundObjectDistanceRolloffModel(
          sound->id, sound->params.spatial_rolloff_method,
          sound->params.spatial_rolloff_min_distance,
          sound->params.spatial_rolloff_max_distance);
    }
    UpdateSoundTransform(sound, sqt);
    audio_->SetSoundVolume(new_id, sound->params.volume);
    audio_->PlaySound(new_id, sound->params.loop);
  } else {
    // Never try to play a failed sound again.
    LOG(ERROR) << "Failed to play sound: " << asset->GetFilename();
    asset->SetLoadStatus(SoundAsset::LoadStatus::kFailed);
  }
}

void AudioSystem::UpdateSoundTransform(Sound* sound, const Sqt& sqt) {
  if (sound->params.source_type ==
      AudioSourceType::AudioSourceType_SoundObject) {
    audio_->SetSoundObjectPosition(sound->id, sqt.translation.x,
                                   sqt.translation.y, sqt.translation.z);
    if (IsSpatialDirectivityEnabled(sound)) {
      audio_->SetSoundObjectRotation(sound->id,
                                     GvrQuatFromMathfu(sqt.rotation));
    }
  } else if (sound->params.source_type ==
             AudioSourceType::AudioSourceType_Soundfield) {
    audio_->SetSoundfieldRotation(sound->id, GvrQuatFromMathfu(sqt.rotation));
  }
}

void AudioSystem::LoadSound(const std::string& filename, AudioLoadType type,
                            Entity e) {
  asset_manager_->CreateSoundAsset(filename, type, e);
}

void AudioSystem::UnloadSound(const std::string& filename) {
  asset_manager_->ReleaseSoundAsset(Hash(filename));
}

void AudioSystem::OnEntityEnabled(Entity entity) {
  // Resume sounds that were playing when the Entity was disabled. Leave paused
  // sounds paused.
  auto* source = sources_.Get(entity);
  if (source && !source->enabled) {
    source->enabled = true;
    if (audio_) {
      for (auto& iter : source->sounds) {
        Sound& sound = iter.second;
        if (!sound.paused) {
          audio_->ResumeSound(sound.id);
        }
      }
    }
  }
}

void AudioSystem::OnEntityDisabled(Entity entity) {
  auto* source = sources_.Get(entity);
  if (source && source->enabled) {
    source->enabled = false;
    if (audio_) {
      for (auto iter = source->sounds.begin(); iter != source->sounds.end();) {
        Sound& sound = iter->second;
        // Looping sounds should be paused so that they may be resumed
        // OnEntityEnabled at the time they were stopped. This includes
        // externally tracked sounds.
        if (sound.params.loop) {
          if (audio_->IsSoundPlaying(sound.id)) {
            audio_->PauseSound(sound.id);
          }
          ++iter;
        } else {
          // Non-looped sounds are forgotten. They will continue playback if not
          // finished.
          iter = source->sounds.erase(iter);
        }
      }
    }
  }
}

}  // namespace lull
