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

#ifndef REDUX_ENGINES_PHYSICS_COLLISION_SHAPE_H_
#define REDUX_ENGINES_PHYSICS_COLLISION_SHAPE_H_

#include <memory>

namespace redux {

// The shape to be used for trigger volumes or rigid bodies.
//
// It is created by the PhysicsEngine using CollisionData. It has no
// functionality of its own.
class CollisionShape {
 public:
  virtual ~CollisionShape() = default;

 protected:
  CollisionShape() = default;
};

using CollisionShapePtr = std::shared_ptr<CollisionShape>;

}  // namespace redux

#endif  // REDUX_ENGINES_PHYSICS_COLLISION_SHAPE_H_
