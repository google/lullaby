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

#include "lullaby/modules/script/lull/functions/functions.h"
#include "lullaby/modules/script/lull/script_env.h"
#include "lullaby/modules/script/lull/script_frame.h"
#include "lullaby/modules/script/lull/script_types.h"
#include "mathfu/glsl_mappings.h"

// This file implements the following script functions:
//
// (vec2i [x] [y])
//   Creates a mathfu::vec2i with the specified integer values.
//
// (vec3i [x] [y] [z])
//   Creates a mathfu::vec3i with the specified integer values.
//
// (vec4i [x] [y] [z] [w])
//   Creates a mathfu::vec4i with the specified integer values.
//
// (vec2 [x] [y])
//   Creates a mathfu::vec2 with the specified floating-point values.
//
// (vec3 [x] [y] [z])
//   Creates a mathfu::vec3 with the specified floating-point values.
//
// (vec4 [x] [y] [z] [w])
//   Creates a mathfu::vec4 with the specified floating-pointinteger values.
//
// (quat [w] [x] [y] [z])
//   Creates a mathfu::quat with the specified floating-pointinteger values.
//   Note: the scalar 'w' value is the first argument to the function.
//
// (get-x [mathfu-value])
//   Returns the x-component of the specified vector/quat.
//
// (get-y [mathfu-value])
//   Returns the y-component of the specified vector/quat.
//
// (get-z [mathfu-value])
//   Returns the z-component of the specified vector/quat.
//
// (get-w [mathfu-value])
//   Returns the w-component of the specified vector/quat.
//
// (set-x [mathfu-value] [value])
//   Sets the x-component of the specified vector/quat to the given value.
//
// (set-y [mathfu-value] [value])
//   Sets the y-component of the specified vector/quat to the given value.
//
// (set-z [mathfu-value] [value])
//   Sets the z-component of the specified vector/quat to the given value.
//
// (set-w [mathfu-value] [value])
//   Sets the w-component of the specified vector/quat to the given value.

namespace lull {
namespace {

using mathfu::quat;
using mathfu::vec2;
using mathfu::vec2i;
using mathfu::vec3;
using mathfu::vec3i;
using mathfu::vec4;
using mathfu::vec4i;

vec2 Vec2Create(float x, float y) { return vec2(x, y); }
vec3 Vec3Create(float x, float y, float z) { return vec3(x, y, z); }
vec4 Vec4Create(float x, float y, float z, float w) { return vec4(x, y, z, w); }
vec2i Vec2iCreate(int x, int y) { return vec2i(x, y); }
vec3i Vec3iCreate(int x, int y, int z) { return vec3i(x, y, z); }
vec4i Vec4iCreate(int x, int y, int z, int w) { return vec4i(x, y, z, w); }
quat QuatCreate(float x, float y, float z, float w) { return quat(x, y, z, w); }

Variant GetX(ScriptFrame* frame, const Variant* vec) {
  if (const vec2* v2 = vec->Get<vec2>()) return v2->x;
  if (const vec3* v3 = vec->Get<vec3>()) return v3->x;
  if (const vec4* v4 = vec->Get<vec4>()) return v4->x;
  if (const vec2i* v2i = vec->Get<vec2i>()) return v2i->x;
  if (const vec3i* v3i = vec->Get<vec3i>()) return v3i->x;
  if (const vec4i* v4i = vec->Get<vec4i>()) return v4i->x;
  if (const quat* qt = vec->Get<quat>()) return qt->vector().x;
  frame->Error("get-x: arg was not a mathfu type");
  return Variant();
}

Variant GetY(ScriptFrame* frame, const Variant* vec) {
  if (const vec2* v2 = vec->Get<vec2>()) return v2->y;
  if (const vec3* v3 = vec->Get<vec3>()) return v3->y;
  if (const vec4* v4 = vec->Get<vec4>()) return v4->y;
  if (const vec2i* v2i = vec->Get<vec2i>()) return v2i->y;
  if (const vec3i* v3i = vec->Get<vec3i>()) return v3i->y;
  if (const vec4i* v4i = vec->Get<vec4i>()) return v4i->y;
  if (const quat* qt = vec->Get<quat>()) return qt->vector().y;
  frame->Error("get-y: arg was not a mathfu type");
  return Variant();
}

Variant GetZ(ScriptFrame* frame, const Variant* vec) {
  if (const vec3* v3 = vec->Get<vec3>()) return v3->z;
  if (const vec4* v4 = vec->Get<vec4>()) return v4->z;
  if (const vec3i* v3i = vec->Get<vec3i>()) return v3i->z;
  if (const vec4i* v4i = vec->Get<vec4i>()) return v4i->z;
  if (const quat* qt = vec->Get<quat>()) return qt->vector().z;
  frame->Error("get-z: arg was not a 3d or 4d mathfu type");
  return Variant();
}

Variant GetW(ScriptFrame* frame, const Variant* vec) {
  if (const vec4* v4 = vec->Get<vec4>()) return v4->w;
  if (const vec4i* v4i = vec->Get<vec4i>()) return v4i->w;
  if (const quat* qt = vec->Get<quat>()) return qt->scalar();
  frame->Error("get-w: arg was not a 4d mathfu type");
  return Variant();
}

void SetX(ScriptFrame* frame, Variant* vec, const Variant* num) {
  Optional<float> f = num->NumericCast<float>();
  if (!f) {
    frame->Error("set-x: 2nd arg was not numeric");
    return;
  }
  if (vec2* v2 = vec->Get<vec2>()) {
    v2->x = *f;
    return;
  }
  if (vec3* v3 = vec->Get<vec3>()) {
    v3->x = *f;
    return;
  }
  if (vec4* v4 = vec->Get<vec4>()) {
    v4->x = *f;
    return;
  }
  if (quat* qt = vec->Get<quat>()) {
    qt->vector().x = *f;
    return;
  }
  Optional<int> i = num->NumericCast<int>();
  if (vec2i* v2i = vec->Get<vec2i>()) {
    v2i->x = *i;
    return;
  }
  if (vec3i* v3i = vec->Get<vec3i>()) {
    v3i->x = *i;
    return;
  }
  if (vec4i* v4i = vec->Get<vec4i>()) {
    v4i->x = *i;
    return;
  }
  frame->Error("set-x: 1st arg was not a mathfu type");
}

void SetY(ScriptFrame* frame, Variant* vec, const Variant* num) {
  Optional<float> f = num->NumericCast<float>();
  if (!f) {
    frame->Error("set-y: 2nd arg was not numeric");
    return;
  }
  if (vec2* v2 = vec->Get<vec2>()) {
    v2->y = *f;
    return;
  }
  if (vec3* v3 = vec->Get<vec3>()) {
    v3->y = *f;
    return;
  }
  if (vec4* v4 = vec->Get<vec4>()) {
    v4->y = *f;
    return;
  }
  if (quat* qt = vec->Get<quat>()) {
    qt->vector().y = *f;
    return;
  }
  Optional<int> i = num->NumericCast<int>();
  if (vec2i* v2i = vec->Get<vec2i>()) {
    v2i->y = *i;
    return;
  }
  if (vec3i* v3i = vec->Get<vec3i>()) {
    v3i->y = *i;
    return;
  }
  if (vec4i* v4i = vec->Get<vec4i>()) {
    v4i->y = *i;
    return;
  }
  frame->Error("set-y: 1st arg was not a mathfu type");
}

void SetZ(ScriptFrame* frame, Variant* vec, const Variant* num) {
  Optional<float> f = num->NumericCast<float>();
  if (!f) {
    frame->Error("set-z: 2nd arg was not numeric");
    return;
  }
  if (vec3* v3 = vec->Get<vec3>()) {
    v3->z = *f;
    return;
  }
  if (vec4* v4 = vec->Get<vec4>()) {
    v4->z = *f;
    return;
  }
  if (quat* qt = vec->Get<quat>()) {
    qt->vector().z = *f;
    return;
  }
  Optional<int> i = num->NumericCast<int>();
  if (vec3i* v3i = vec->Get<vec3i>()) {
    v3i->z = *i;
    return;
  }
  if (vec4i* v4i = vec->Get<vec4i>()) {
    v4i->z = *i;
    return;
  }
  frame->Error("set-z: 1st arg was not a mathfu type");
}

void SetW(ScriptFrame* frame, Variant* vec, const Variant* num) {
  Optional<float> f = num->NumericCast<float>();
  if (!f) {
    frame->Error("set-w: 2nd arg was not numeric");
    return;
  }
  if (vec4* v4 = vec->Get<vec4>()) {
    v4->w = *f;
    return;
  }
  if (quat* qt = vec->Get<quat>()) {
    qt->scalar() = *f;
    return;
  }
  Optional<int> i = num->NumericCast<int>();
  if (vec4i* v4i = vec->Get<vec4i>()) {
    v4i->w = *i;
    return;
  }
  frame->Error("set-w: 1st arg was not a mathfu type");
}

LULLABY_SCRIPT_FUNCTION_WRAP(Vec2Create, "vec2");
LULLABY_SCRIPT_FUNCTION_WRAP(Vec3Create, "vec3");
LULLABY_SCRIPT_FUNCTION_WRAP(Vec4Create, "vec4");
LULLABY_SCRIPT_FUNCTION_WRAP(Vec2iCreate, "vec2i");
LULLABY_SCRIPT_FUNCTION_WRAP(Vec3iCreate, "vec3i");
LULLABY_SCRIPT_FUNCTION_WRAP(Vec4iCreate, "vec4i");
LULLABY_SCRIPT_FUNCTION_WRAP(QuatCreate, "quat");
LULLABY_SCRIPT_FUNCTION_WRAP(GetX, "get-x");
LULLABY_SCRIPT_FUNCTION_WRAP(GetY, "get-y");
LULLABY_SCRIPT_FUNCTION_WRAP(GetZ, "get-z");
LULLABY_SCRIPT_FUNCTION_WRAP(GetW, "get-w");
LULLABY_SCRIPT_FUNCTION_WRAP(SetX, "set-x");
LULLABY_SCRIPT_FUNCTION_WRAP(SetY, "set-y");
LULLABY_SCRIPT_FUNCTION_WRAP(SetZ, "set-z");
LULLABY_SCRIPT_FUNCTION_WRAP(SetW, "set-w");

}  // namespace
}  // namespace lull
