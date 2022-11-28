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

#include "redux/engines/audio/resonance/resonance_utils.h"

#include "platforms/common/room_properties.h"

namespace redux {

vraudio::MaterialName ToResonance(AudioSurfaceMaterial type) {
  switch (type) {
    case AudioSurfaceMaterial::Transparent:
      return vraudio::MaterialName::kTransparent;
    case AudioSurfaceMaterial::AcousticCeilingTiles:
      return vraudio::MaterialName::kAcousticCeilingTiles;
    case AudioSurfaceMaterial::BrickBare:
      return vraudio::MaterialName::kBrickBare;
    case AudioSurfaceMaterial::BrickPainted:
      return vraudio::MaterialName::kBrickPainted;
    case AudioSurfaceMaterial::ConcreteBlockCoarse:
      return vraudio::MaterialName::kConcreteBlockCoarse;
    case AudioSurfaceMaterial::ConcreteBlockPainted:
      return vraudio::MaterialName::kConcreteBlockPainted;
    case AudioSurfaceMaterial::CurtainHeavy:
      return vraudio::MaterialName::kCurtainHeavy;
    case AudioSurfaceMaterial::FiberGlassInsulation:
      return vraudio::MaterialName::kFiberGlassInsulation;
    case AudioSurfaceMaterial::GlassThin:
      return vraudio::MaterialName::kGlassThin;
    case AudioSurfaceMaterial::GlassThick:
      return vraudio::MaterialName::kGlassThick;
    case AudioSurfaceMaterial::Grass:
      return vraudio::MaterialName::kGrass;
    case AudioSurfaceMaterial::LinoleumOnConcrete:
      return vraudio::MaterialName::kLinoleumOnConcrete;
    case AudioSurfaceMaterial::Marble:
      return vraudio::MaterialName::kMarble;
    case AudioSurfaceMaterial::Metal:
      return vraudio::MaterialName::kMetal;
    case AudioSurfaceMaterial::ParquetOnConcrete:
      return vraudio::MaterialName::kParquetOnConcrete;
    case AudioSurfaceMaterial::PlasterRough:
      return vraudio::MaterialName::kPlasterRough;
    case AudioSurfaceMaterial::PlasterSmooth:
      return vraudio::MaterialName::kPlasterSmooth;
    case AudioSurfaceMaterial::PlywoodPanel:
      return vraudio::MaterialName::kPlywoodPanel;
    case AudioSurfaceMaterial::PolishedConcreteOrTile:
      return vraudio::MaterialName::kPolishedConcreteOrTile;
    case AudioSurfaceMaterial::Sheetrock:
      return vraudio::MaterialName::kSheetrock;
    case AudioSurfaceMaterial::WaterOrIceSurface:
      return vraudio::MaterialName::kWaterOrIceSurface;
    case AudioSurfaceMaterial::WoodCeiling:
      return vraudio::MaterialName::kWoodCeiling;
    case AudioSurfaceMaterial::WoodPanel:
      return vraudio::MaterialName::kWoodPanel;
    default:
      LOG(FATAL) << "Unknown surface material";
  }
  return vraudio::MaterialName::kTransparent;
}

vraudio::DistanceRolloffModel ToResonance(Sound::DistanceRolloffModel model) {
  switch (model) {
    case Sound::NoRollof:
      return vraudio::DistanceRolloffModel::kNone;
    case Sound::LinearRolloff:
      return vraudio::DistanceRolloffModel::kLinear;
    case Sound::LogarithmicRolloff:
      return vraudio::DistanceRolloffModel::kLogarithmic;
    default:
      CHECK(false);
      return vraudio::DistanceRolloffModel::kNone;
  }
}

vraudio::RoomProperties ToResonance(const SoundRoom& room, const vec3& position,
                                    const quat& rotation) {
  vraudio::RoomProperties resonance;
  resonance.position[0] = position.x;
  resonance.position[1] = position.y;
  resonance.position[2] = position.z;

  resonance.rotation[0] = rotation.x;
  resonance.rotation[1] = rotation.y;
  resonance.rotation[2] = rotation.z;
  resonance.rotation[3] = rotation.w;

  resonance.dimensions[0] = room.size.x;
  resonance.dimensions[1] = room.size.y;
  resonance.dimensions[2] = room.size.z;

  for (int i = 0; i < SoundRoom::kNumWalls; ++i) {
    resonance.material_names[i] = ToResonance(room.surface_materials[i]);
  }

  resonance.reverb_gain = room.reverb_gain;
  resonance.reverb_time = room.reverb_time;
  resonance.reverb_brightness = room.reverb_brightness;
  return resonance;
}
}  // namespace redux
