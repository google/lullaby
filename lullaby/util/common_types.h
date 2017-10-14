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

#ifndef LULLABY_BASE_COMMON_TYPES_H_
#define LULLABY_BASE_COMMON_TYPES_H_

#include <string>

#include "mathfu/glsl_mappings.h"
#include "lullaby/util/color.h"
#include "lullaby/util/math.h"
#include "lullaby/util/typeid.h"

LULLABY_SETUP_TYPEID(bool);
LULLABY_SETUP_TYPEID(int8_t);
LULLABY_SETUP_TYPEID(uint8_t);
LULLABY_SETUP_TYPEID(int16_t);
LULLABY_SETUP_TYPEID(uint16_t);
LULLABY_SETUP_TYPEID(int32_t);
LULLABY_SETUP_TYPEID(uint32_t);
LULLABY_SETUP_TYPEID(int64_t);
LULLABY_SETUP_TYPEID(uint64_t);
LULLABY_SETUP_TYPEID(float);
LULLABY_SETUP_TYPEID(double);
LULLABY_SETUP_TYPEID(std::string);
LULLABY_SETUP_TYPEID(mathfu::vec2);
LULLABY_SETUP_TYPEID(mathfu::vec2i);
LULLABY_SETUP_TYPEID(mathfu::vec3);
LULLABY_SETUP_TYPEID(mathfu::vec3i);
LULLABY_SETUP_TYPEID(mathfu::vec4);
LULLABY_SETUP_TYPEID(mathfu::vec4i);
LULLABY_SETUP_TYPEID(mathfu::quat);
LULLABY_SETUP_TYPEID(mathfu::mat4);
LULLABY_SETUP_TYPEID(lull::Aabb);
LULLABY_SETUP_TYPEID(lull::Color4ub);

#ifdef __APPLE__
LULLABY_SETUP_TYPEID(size_t);
#endif

#endif  // LULLABY_BASE_COMMON_TYPES_H_
