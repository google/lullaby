/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_SYSTEMS_SHAPE_SHAPE_SYSTEM_H_
#define LULLABY_SYSTEMS_SHAPE_SHAPE_SYSTEM_H_

#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/generated/shape_def_generated.h"

namespace lull {

// The ShapeSystem generates common mesh shapes for entities.
class ShapeSystem : public System {
 public:
  explicit ShapeSystem(Registry* registry);

  ShapeSystem(const ShapeSystem&) = delete;
  ShapeSystem& operator=(const ShapeSystem&) = delete;

  void PostCreateComponent(Entity entity, const Blueprint& blueprint) override;

  void CreateRect(Entity entity, const RectMeshDefT& rect);
  void CreateSphere(Entity entity, const SphereDefT& sphere);

 private:
  void CreateShape(Entity entity, HashValue pass, MeshData mesh_data);
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ShapeSystem);

#endif  // LULLABY_SYSTEMS_SHAPE_SHAPE_SYSTEM_H_
