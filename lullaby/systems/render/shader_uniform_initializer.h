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

#ifndef LULLABY_SYSTEMS_RENDER_SHADER_UNIFORM_INITIALIZER_H_
#define LULLABY_SYSTEMS_RENDER_SHADER_UNIFORM_INITIALIZER_H_

#include "lullaby/util/entity.h"
#include "lullaby/util/registry.h"

// Utility functions for initializing shader uniforms that have not been set
// already (e.g. in jsonnet). One can initialize uniforms to zeros or to
// specific values.
//
// While uniforms are supposed to be automatically initialized to zero by the
// driver, some drivers are not consistent about this, so it is safest to
// initialize everything explicitly.

namespace lull {

// Initializes single uniform of given dimension to zero(s).
//
// InitializeUniform(entity, "fake_env_sky_color", 3); sets a vec3 to zero.
void InitializeUniform(Registry* registry, Entity entity,
                       const char* uniform_name, int dimension);
// Initializes array of count uniforms of given dimension to zero(s).
//
// E.g. InitializeUniform(entity, "contact_shadow_points", 4, 160); sets
// 160 vec4's to zero.

void InitializeUniform(Registry* registry, Entity entity,
                       const char* uniform_name, int dimension, int count);
// Initializes single uniform of some dimension to values in the initializer
// list, where the length of the initializer list determines dimension.
//
// E.g. InitializeUniform(entity, "uv_scale_offset", {1.f, 1.f, 0.f, 0.f});
// sets a vec4 to (1, 1, 0, 0).
void InitializeUniform(Registry* registry, Entity entity,
                       const char* uniform_name,
                       std::initializer_list<float> values);

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_SHADER_UNIFORM_INITIALIZER_H_
