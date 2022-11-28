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

#ifndef REDUX_ENGINES_ANIMATION_ANIMATION_ENGINE_H_
#define REDUX_ENGINES_ANIMATION_ANIMATION_ENGINE_H_

#include <functional>
#include <memory>
#include <string_view>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "redux/engines/animation/animation_clip.h"
#include "redux/engines/animation/common.h"
#include "redux/engines/animation/processor/anim_processor.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/base/resource_manager.h"
#include "redux/modules/base/typeid.h"

namespace redux {

// Holds and updates all animation data.
//
// The engine holds all of the AnimProcessors, and updates them all when
// AdvanceFrame() is called. The processing is kept central, in this manner,
// for scalability. The engine is not a singleton, but you should try to
// minimize the number of engines. As more Motivators are added to the
// processors, you start to get economies of scale.
class AnimationEngine {
 public:
  AnimationEngine(const AnimationEngine&) = delete;
  AnimationEngine& operator=(const AnimationEngine&) = delete;

  // Creates an instance of the AnimationEngine and adds it to the registry.
  static void Create(Registry* registry);

  void OnRegistryInitialize();

  // Update all the AnimProcessors by 'delta_time'. This advances all Motivators
  // created with this AnimationEngine.
  void AdvanceFrame(absl::Duration delta_time);

  // Registers a Motivator and Processor with the Engine. Motivators can then be
  // created using AcquireMotivator which will then use the specified Processor
  // to animate the underlying values.
  template <typename MotivatorT, typename ProcessorT>
  void RegisterMotivator() {
    const TypeId processor_type_id = GetTypeId<ProcessorT>();
    const TypeId motivator_type_id = GetTypeId<MotivatorT>();

    auto ptr = new ProcessorT(this);
    processors_[processor_type_id].reset(ptr);
    allocators_[motivator_type_id] = [ptr](Motivator* motivator,
                                           int dimensions) {
      MotivatorT m = ptr->AllocateMotivator(dimensions);
      CHECK(sizeof(MotivatorT) == sizeof(MotivatorT));
      *motivator = std::move(m);
    };

    sorted_processors_.clear();
  }

  // Acquires a Motivator that can be used to control and access a value that
  // is being animated by the AnimationEngine.
  template <typename MotivatorT>
  MotivatorT AcquireMotivator(int dimensions = 1) {
    MotivatorT motivator;
    auto iter = allocators_.find(GetTypeId<MotivatorT>());
    if (iter != allocators_.end()) {
      iter->second(&motivator, dimensions);
    }
    CHECK(motivator.Valid()) << "Did you register this motivator?";
    return motivator;
  }

  // Loads and returns the animation data file at the given uri.
  AnimationClipPtr LoadAnimationClip(std::string_view uri);

  // Returns an animation data file that has been previously loaded. The `key`
  // will be the Hash of the `uri`. Returns nullptr if the clip has been
  // unloaded which happens when all references to this clip are released.
  AnimationClipPtr GetAnimationClip(HashValue key);

 private:
  explicit AnimationEngine(Registry* registry) : registry_(registry) {}

  using ProcessorPtr = std::unique_ptr<AnimProcessor>;
  using AllocateMotivatorFn = std::function<void(Motivator*, int)>;

  Registry* registry_ = nullptr;
  absl::flat_hash_map<TypeId, ProcessorPtr> processors_;
  absl::flat_hash_map<TypeId, AllocateMotivatorFn> allocators_;
  std::vector<AnimProcessor*> sorted_processors_;
  ResourceManager<AnimationClip> animation_clips_;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::AnimationEngine);

#endif  // REDUX_ENGINES_ANIMATION_ANIMATION_ENGINE_H_
