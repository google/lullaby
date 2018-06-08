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

#include "lullaby/systems/render/detail/uniform_linker.h"

#include <utility>

#include "lullaby/util/logging.h"

namespace lull {
namespace detail {
namespace {
void DefaultCopy(const float* source_data, int dimension, int count,
                 float* target_data) {
  if (!target_data) {
    LOG(DFATAL) << "Invalid target_data.";
    return;
  }
  memcpy(target_data, source_data, dimension * count * sizeof(float));
};
}  // namespace

UniformLinker::TargetComponent* UniformLinker::GetOrCreateLink(Entity target,
                                                               Entity source) {
  auto target_pair = targets_.find(target);
  if (target_pair != targets_.end()) {
    TargetComponent* target_component = &target_pair->second;
    if (target_component->source == kNullEntity) {
      target_component->source = source;
      sources_[source].targets.emplace(target);
      return target_component;
    }
    if (target_component->source == source) {
      return target_component;
    }

    LOG(WARNING) << "A link already exists for target " << target
                 << " with source " << target_component->source
                 << ", overwriting with source " << source << ".";
    RemoveTargetFromSource(target, target_component->source);
    targets_[target] = TargetComponent();
  }
  sources_[source].targets.emplace(target);
  targets_[target].source = source;
  return &targets_[target];
}

void UniformLinker::RemoveTargetFromSource(Entity target, Entity source) {
  auto pair = sources_.find(source);
  if (pair != sources_.end()) {
    SourceComponent& source_component = pair->second;
    source_component.targets.erase(target);
    if (source_component.targets.empty()) {
      sources_.erase(source);
    }
  }
}

void UniformLinker::LinkUniform(Entity target, Entity source,
                                HashValue name_hash,
                                UpdateUniformFn update_fn) {
  TargetComponent* target_component = GetOrCreateLink(target, source);
  target_component->link_uniform_fns[name_hash] = std::move(update_fn);
}

void UniformLinker::LinkAllUniforms(Entity target, Entity source,
                                    UpdateUniformFn update_fn) {
  TargetComponent* target_component = GetOrCreateLink(target, source);
  target_component->link_all_uniforms_fn = std::move(update_fn);
}

void UniformLinker::IgnoreLinkedUniform(Entity target, HashValue name_hash) {
  targets_[target].ignored_uniforms.emplace(name_hash);
}

void UniformLinker::UnlinkUniforms(Entity entity) {
  const auto target_pair = targets_.find(entity);
  if (target_pair != targets_.end()) {
    TargetComponent& target_component = target_pair->second;
    if (target_component.source != kNullEntity) {
      RemoveTargetFromSource(entity, target_component.source);
    }
    targets_.erase(entity);
  }

  const auto source_pair = sources_.find(entity);
  if (source_pair != sources_.end()) {
    for (Entity target : source_pair->second.targets) {
      targets_.erase(target);
    }
    sources_.erase(entity);
  }
}

void UniformLinker::UpdateLinkedUniforms(
    Entity source, HashValue name_hash, const float* data, int dimension,
    int count, const GetUniformDataFn& get_data_fn) {
  const auto source_pair = sources_.find(source);
  if (source_pair == sources_.end()) {
    return;
  }
  for (Entity target : source_pair->second.targets) {
    const auto target_pair = targets_.find(target);
    if (target_pair == targets_.end()) {
      LOG(WARNING) << "Target " << target << " not found for source " << source;
      continue;
    }

    const TargetComponent& target_component = target_pair->second;
    if (target_component.ignored_uniforms.count(name_hash)) {
      continue;
    }
    const auto fn_pair = target_component.link_uniform_fns.find(name_hash);
    const UpdateUniformFn* update_fn = nullptr;
    if (fn_pair != target_component.link_uniform_fns.end()) {
      update_fn = &fn_pair->second;
    } else if (target_component.link_all_uniforms_fn) {
      update_fn = target_component.link_all_uniforms_fn.get();
    } else {
      // This case occurs if there is a LinkUniform link, no LinkAllUniforms
      // link, and was updated with a different name_hash.
      continue;
    }
    if (*update_fn) {
      (*update_fn)(data, dimension, count,
                   get_data_fn(target, dimension, count));
    } else {
      DefaultCopy(data, dimension, count,
                  get_data_fn(target, dimension, count));
    }
  }
}

}  // namespace detail
}  // namespace lull
