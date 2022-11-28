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

#include "redux/engines/animation/animation_engine.h"

#include <utility>

#include "absl/algorithm/container.h"
#include "absl/time/time.h"
#include "redux/engines/animation/motivator/rig_motivator.h"
#include "redux/engines/animation/motivator/spline_motivator.h"
#include "redux/engines/animation/motivator/transform_motivator.h"
#include "redux/engines/animation/processor/rig_processor.h"
#include "redux/engines/animation/processor/spline_processor.h"
#include "redux/engines/animation/processor/transform_processor.h"
#include "redux/modules/base/asset_loader.h"
#include "redux/modules/base/choreographer.h"
#include "redux/modules/base/static_registry.h"

namespace redux {

void AnimationEngine::Create(Registry* registry) {
  auto ptr = new AnimationEngine(registry);
  registry->Register(std::unique_ptr<AnimationEngine>(ptr));

  ptr->RegisterMotivator<SplineMotivator, SplineProcessor>();
  ptr->RegisterMotivator<TransformMotivator, TransformProcessor>();
  ptr->RegisterMotivator<RigMotivator, RigProcessor>();
}

void AnimationEngine::OnRegistryInitialize() {
  auto* choreographer = registry_->Get<Choreographer>();
  if (choreographer) {
    choreographer->Add<&AnimationEngine::AdvanceFrame>(
        Choreographer::Stage::kAnimation);
  }
}

void AnimationEngine::AdvanceFrame(absl::Duration delta_time) {
  // Advance the simulation in each processor.
  // TODO: At some point, we'll want to do several passes. An item in
  // processor A might depend on the output of an item in processor B,
  // which might in turn depend on the output of a *different* item in
  // processor A. In this case, we have to do two passes. For now, just
  // assume that one pass is sufficient.
  if (sorted_processors_.empty()) {
    sorted_processors_.reserve(processors_.size());
    for (auto& iter : processors_) {
      sorted_processors_.emplace_back(iter.second.get());
    }
    absl::c_sort(sorted_processors_, [](AnimProcessor* a, AnimProcessor* b) {
      return a->Priority() < b->Priority();
    });
  }
  for (AnimProcessor* processor : sorted_processors_) {
    processor->AdvanceFrame(delta_time);
  }
}

AnimationClipPtr AnimationEngine::LoadAnimationClip(std::string_view uri) {
  const HashValue key = Hash(uri);
  AnimationClipPtr clip = animation_clips_.Find(key);
  if (clip == nullptr) {
    clip = std::make_shared<AnimationClip>();
    auto on_load = [=](AssetLoader::StatusOrData& asset) mutable {
      if (asset.ok()) {
        clip->Initialize(std::move(*asset));
      }
    };
    auto on_finalize = [=](AssetLoader::StatusOrData& asset) mutable {
      clip->Finalize();
    };

    auto asset_loader = registry_->Get<AssetLoader>();
    asset_loader->LoadAsync(uri, on_load, on_finalize);
    animation_clips_.Register(key, clip);
  }
  return clip;
}

AnimationClipPtr AnimationEngine::GetAnimationClip(HashValue key) {
  return animation_clips_.Find(key);
}

static StaticRegistry Static_Register(AnimationEngine::Create);

}  // namespace redux
