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

#ifndef LULLABY_SYSTEMS_NINE_PATCH_NINE_PATCH_SYSTEM_H_
#define LULLABY_SYSTEMS_NINE_PATCH_NINE_PATCH_SYSTEM_H_

#include "lullaby/events/layout_events.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/render/nine_patch.h"
#include "lullaby/util/optional.h"

namespace lull {

// The NinePatchSystem provides a component for rendering nine patches. Given
// the dimensions of a quad to fill, the original (unaltered) size of the nine
// patch, and the locations of the slices, a mesh is generated with appropriate
// vertex locations and texture coordinates.
class NinePatchSystem : public System {
 public:
  explicit NinePatchSystem(Registry* registry);

  ~NinePatchSystem() override;

  // Adds a nine patch mesh to |entity| using the specified NinePatchDef.
  void PostCreateInit(Entity entity, DefType type, const Def* def) override;

  // Removes the nine patch mesh from |entity|.
  void Destroy(Entity entity) override;

  // Returns the size of the nine patch geometry or an empty optional if the
  // entity does not have a nine-patch component.
  Optional<mathfu::vec2> GetSize(Entity entity) const;

 private:
  // Recomputes the mesh for |entity| given the component data in |nine_patch|.
  void UpdateNinePatchMesh(Entity entity, Entity source,
                           const NinePatch* nine_patch);

  // Recompute nine_patch based on new desired_size.
  void OnDesiredSizeChanged(const DesiredSizeChangedEvent& event);

  std::unordered_map<Entity, NinePatch> nine_patches_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::NinePatchSystem);

#endif  // LULLABY_SYSTEMS_NINE_PATCH_NINE_PATCH_SYSTEM_H_
