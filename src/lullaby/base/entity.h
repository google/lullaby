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

#ifndef LULLABY_BASE_ENTITY_H_
#define LULLABY_BASE_ENTITY_H_

namespace lull {

// Entity definition for Lullaby's Entity-Component-System (ECS) architecture.
//
// An Entity represents each uniquely identifiable object in the Lullaby
// runtime.  An Entity itself does not have any data or functionality - it is
// just a way to uniquely identify objects and is simply a number.
using Entity = unsigned int;

// Special Entity value used for invalid Entities.
static const Entity kNullEntity = 0;

}  // namespace lull

#endif  // LULLABY_BASE_ENTITY_H_
