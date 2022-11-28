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

#ifndef REDUX_SYSTEMS_SHAPE_SHAPE_SYSTEM_H_
#define REDUX_SYSTEMS_SHAPE_SHAPE_SYSTEM_H_

#include "redux/modules/ecs/system.h"
#include "redux/modules/math/bounds.h"
#include "redux/systems/shape/shape_def_generated.h"

namespace redux {

// Creates simple shapes for the Physics and Render Systems.
class ShapeSystem : public System {
 public:
  explicit ShapeSystem(Registry* registry);

  // Creates a box shape using the BoxShapeDef.
  void AddBoxShapeDef(Entity entity, const BoxShapeDef& def);

  // Creates a sphere shape using the SphereShapeDef.
  void AddSphereShapeDef(Entity entity, const SphereShapeDef& def);

 private:
  template <typename T>
  void BuildCollisionShape(Entity entity, const T* def);

  template <typename T>
  void BuildMeshShape(Entity entity, const T* def, const Box& bounds);
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::ShapeSystem);

#endif  // REDUX_SYSTEMS_SHAPE_SHAPE_SYSTEM_H_
