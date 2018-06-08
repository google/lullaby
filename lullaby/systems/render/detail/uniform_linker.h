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

#ifndef LULLABY_SYSTEMS_RENDER_DETAIL_UNIFORM_LINKER_H_
#define LULLABY_SYSTEMS_RENDER_DETAIL_UNIFORM_LINKER_H_

#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "lullaby/util/entity.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/optional.h"

namespace lull {
namespace detail {

/// A helper class to manage linked uniforms. This stores pairs of entities,
/// source and target, and will perform updates on targets when sources are
/// updated.
class UniformLinker {
 public:
  /// Callback used to update uniforms. It passes in the mutable |target_data|
  /// pointer so that the update function can read and write all its output
  /// directly there.
  using UpdateUniformFn = std::function<void(
      const float* source_data, int dimension, int count, float* target_data)>;

  /// Whenever uniform |name_hash| on |source| is updated, also update |target|
  /// with |update_fn|. This takes precedence over the update_fn from
  /// LinkAllUniforms(). If |update_fn| is the null default it will perform a
  /// simple memcpy. Ignored uniforms are not updated. The same |target| cannot
  /// be linked from multiple different |sources|.
  void LinkUniform(Entity target, Entity source, HashValue name_hash,
                   UpdateUniformFn update_fn = UpdateUniformFn());

  /// Whenever any uniform on |source| is updated, also update |target| with
  /// |update_fn|. If |update_fn| is the null default it will perform a simple
  /// memcpy. Ignored uniforms are not updated. The same |target| cannot be
  /// linked from multiple different |sources|.
  void LinkAllUniforms(Entity target, Entity source,
                       UpdateUniformFn update_fn = UpdateUniformFn());

  /// Do not let UpdateLinkedUniforms() modify uniform |name_hash| on |target|.
  void IgnoreLinkedUniform(Entity target, HashValue name_hash);

  /// Remove any links to or from |entity|. If |entity| is a source, all of its
  /// targets are also removed. If |entity| is the last target of a source, that
  /// source is also removed. Ignored uniforms are also reset.
  void UnlinkUniforms(Entity entity);

  /// Callback provided by the caller of UpdateLinkedUniforms() that fetches
  /// the mutable target_data pointer for the |target| of a link that will be
  /// modified by the UpdateUniformFn for the link.
  using GetUniformDataFn = std::function<float*(Entity target, int dimension,
                                                int count)>;

  /// For all of |source's| targets, if |name_hash| is linked from
  /// LinkUniform(), call that update_fn. Otherwise, if it was linked from
  /// LinkAllUniforms() then call that update_fn. But, Ignored uniforms are
  /// not updated by either.
  void UpdateLinkedUniforms(
      Entity source, HashValue name_hash, const float* data, int dimension,
      int count, const GetUniformDataFn& get_data_fn);

 private:
  struct SourceComponent {
    std::unordered_set<Entity> targets;
  };
  struct TargetComponent {
    // Used in Destroy() to erase from sources_.targets. This can be kNullEntity
    // when this component only contains some ignored_uniforms.
    Entity source = kNullEntity;
    // Uniforms that will be ignored and not updated.
    std::unordered_set<HashValue> ignored_uniforms;
    // Links established from LinkUniform().
    std::unordered_map<HashValue, UpdateUniformFn> link_uniform_fns;
    // Link established from LinkAllUniforms().
    Optional<UpdateUniformFn> link_all_uniforms_fn;
  };

  // Ensures that every target is only modified by one source. If target already
  // exists with a different source, it is reset and logs a WARNING.
  TargetComponent* GetOrCreateLink(Entity target, Entity source);
  void RemoveTargetFromSource(Entity target, Entity source);

  // For simplicity, we only allow an entity to be the target of one source.
  std::unordered_map<Entity, SourceComponent> sources_;
  std::unordered_map<Entity, TargetComponent> targets_;
};

}  // namespace detail
}  // namespace lull

#endif  /// LULLABY_SYSTEMS_RENDER_DETAIL_UNIFORM_LINKER_H_
