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

// Helper struct that stores ScriptFrame and arguments so that they can be
// used by multiple functions.
struct Args {
  explicit Args(ScriptFrame* frame) : frame(frame) {
    if (frame->HasNext()) {
      arg1 = frame->EvalNext();
    }
    if (frame->HasNext()) {
      arg2 = frame->EvalNext();
    }
  }

  ScriptFrame* frame;
  ScriptValue arg1;
  ScriptValue arg2;
};

// Attempts to evaluate a floating-point value of the ScriptFrame.
Optional<float> TryEvalNextFloatValue(ScriptFrame* frame) {
  if (frame->HasNext()) {
    return frame->EvalNext().NumericCast<float>();
  } else {
    return NullOpt;
  }
}

// Mapping of vector/quaternoin dimensions to indices used by operator[] for
// various mathfu types.
enum Dimension {
  kXValue = 0,
  kYValue = 1,
  kZValue = 2,
  kWValue = 3,
};

// Template function used to create a vector with the specified Scalar type |T|
// and number of Dimensions |D|.
template <class T, int D>
void VectorCreate(ScriptFrame* frame) {
  mathfu::Vector<T, D> vec;
  for (int i = 0; i < D; ++i) {
    if (!frame->HasNext()) {
      frame->Error("Invalid arguments for constructing vector.");
      return;
    }
    const auto value = frame->EvalNext().NumericCast<T>();
    if (!value) {
      frame->Error("Invalid arguments for constructing vector.");
      return;
    }
    vec[i] = *value;
  }
  frame->Return(vec);
}

// Template function that attempts to get the specified Dimensional value from a
// vector. Returns true if successful, false if the arguments are of the wrong
// type.
template <typename T>
bool TryGet(Args* args, Dimension index) {
  auto* vec = args->arg1.Get<T>();
  if (!vec) {
    return false;
  }
  args->frame->Return((*vec)[index]);
  return true;
}

// Specialization of the TryGet function, but for quaternions.
template <>
bool TryGet<mathfu::quat>(Args* args, Dimension index) {
  auto* quat = args->arg1.Get<mathfu::quat>();
  if (!quat) {
    return false;
  }
  if (index == kWValue) {
    args->frame->Return(quat->scalar());
  } else {
    args->frame->Return(quat->vector()[index]);
  }
  return true;
}

// Template function that attempts to set the specified Dimensional value of a
// vector. Returns true if successful, false if the arguments are of the wrong
// type.
template <typename T>
bool TrySet(Args* args, Dimension index) {
  auto* vec = args->arg1.Get<T>();
  if (!vec) {
    return false;
  }
  const auto value = args->arg2.NumericCast<typename T::Scalar>();
  if (!value) {
    return false;
  }
  (*vec)[index] = *value;
  args->frame->Return(*value);
  return true;
}

// Specialization of the TrySet function, but for quaternions.
template <>
bool TrySet<mathfu::quat>(Args* args, Dimension index) {
  auto* quat = args->arg1.Get<mathfu::quat>();
  if (!quat) {
    return false;
  }
  const auto value = args->arg2.NumericCast<float>();
  if (!value) {
    return false;
  }
  if (index == kWValue) {
    quat->scalar() = *value;
  } else {
    quat->vector()[index] = *value;
  }
  args->frame->Return(*value);
  return true;
}

void Vec2Create(ScriptFrame* frame) { VectorCreate<float, 2>(frame); }

void Vec3Create(ScriptFrame* frame) { VectorCreate<float, 3>(frame); }

void Vec4Create(ScriptFrame* frame) { VectorCreate<float, 4>(frame); }

void Vec2iCreate(ScriptFrame* frame) { VectorCreate<int, 2>(frame); }

void Vec3iCreate(ScriptFrame* frame) { VectorCreate<int, 3>(frame); }

void Vec4iCreate(ScriptFrame* frame) { VectorCreate<int, 4>(frame); }

void QuatCreate(ScriptFrame* frame) {
  const mathfu::quat res;
  const auto v1 = TryEvalNextFloatValue(frame);
  const auto v2 = TryEvalNextFloatValue(frame);
  const auto v3 = TryEvalNextFloatValue(frame);
  const auto v4 = TryEvalNextFloatValue(frame);
  if (v1 && v2 && v3 && v4) {
    const mathfu::quat res(*v1, *v2, *v3, *v4);
    frame->Return(res);
  } else {
    frame->Error("Invalid arguments for constructing quaternion.");
  }
}

void GetX(ScriptFrame* frame) {
  Args args(frame);
  if (TryGet<mathfu::vec2>(&args, kXValue)) {
    return;
  } else if (TryGet<mathfu::vec2i>(&args, kXValue)) {
    return;
  } else if (TryGet<mathfu::vec3>(&args, kXValue)) {
    return;
  } else if (TryGet<mathfu::vec3i>(&args, kXValue)) {
    return;
  } else if (TryGet<mathfu::vec4>(&args, kXValue)) {
    return;
  } else if (TryGet<mathfu::vec4i>(&args, kXValue)) {
    return;
  } else if (TryGet<mathfu::quat>(&args, kXValue)) {
    return;
  } else {
    frame->Error("get-x: invalid arguments");
  }
}

void GetY(ScriptFrame* frame) {
  Args args(frame);
  if (TryGet<mathfu::vec2>(&args, kYValue)) {
    return;
  } else if (TryGet<mathfu::vec2i>(&args, kYValue)) {
    return;
  } else if (TryGet<mathfu::vec3>(&args, kYValue)) {
    return;
  } else if (TryGet<mathfu::vec3i>(&args, kYValue)) {
    return;
  } else if (TryGet<mathfu::vec4>(&args, kYValue)) {
    return;
  } else if (TryGet<mathfu::vec4i>(&args, kYValue)) {
    return;
  } else if (TryGet<mathfu::quat>(&args, kYValue)) {
    return;
  } else {
    frame->Error("get-y: invalid arguments");
  }
}

void GetZ(ScriptFrame* frame) {
  Args args(frame);
  if (TryGet<mathfu::vec3>(&args, kZValue)) {
    return;
  } else if (TryGet<mathfu::vec3i>(&args, kZValue)) {
    return;
  } else if (TryGet<mathfu::vec4>(&args, kZValue)) {
    return;
  } else if (TryGet<mathfu::vec4i>(&args, kZValue)) {
    return;
  } else if (TryGet<mathfu::quat>(&args, kZValue)) {
    return;
  } else {
    frame->Error("get-z: invalid arguments");
  }
}

void GetW(ScriptFrame* frame) {
  Args args(frame);
  if (TryGet<mathfu::vec4>(&args, kWValue)) {
    return;
  } else if (TryGet<mathfu::vec4i>(&args, kWValue)) {
    return;
  } else if (TryGet<mathfu::quat>(&args, kWValue)) {
    return;
  } else {
    frame->Error("get-w: invalid arguments");
  }
}

void SetX(ScriptFrame* frame) {
  Args args(frame);
  if (TrySet<mathfu::vec2>(&args, kXValue)) {
    return;
  } else if (TrySet<mathfu::vec2i>(&args, kXValue)) {
    return;
  } else if (TrySet<mathfu::vec3>(&args, kXValue)) {
    return;
  } else if (TrySet<mathfu::vec3i>(&args, kXValue)) {
    return;
  } else if (TrySet<mathfu::vec4>(&args, kXValue)) {
    return;
  } else if (TrySet<mathfu::vec4i>(&args, kXValue)) {
    return;
  } else if (TrySet<mathfu::quat>(&args, kXValue)) {
    return;
  } else {
    frame->Error("set-x: invalid arguments");
  }
}

void SetY(ScriptFrame* frame) {
  Args args(frame);
  if (TrySet<mathfu::vec2>(&args, kYValue)) {
    return;
  } else if (TrySet<mathfu::vec2i>(&args, kYValue)) {
    return;
  } else if (TrySet<mathfu::vec3>(&args, kYValue)) {
    return;
  } else if (TrySet<mathfu::vec3i>(&args, kYValue)) {
    return;
  } else if (TrySet<mathfu::vec4>(&args, kYValue)) {
    return;
  } else if (TrySet<mathfu::vec4i>(&args, kYValue)) {
    return;
  } else if (TrySet<mathfu::quat>(&args, kYValue)) {
    return;
  } else {
    frame->Error("set-y: invalid arguments");
  }
}

void SetZ(ScriptFrame* frame) {
  Args args(frame);
  if (TrySet<mathfu::vec3>(&args, kZValue)) {
    return;
  } else if (TrySet<mathfu::vec3i>(&args, kZValue)) {
    return;
  } else if (TrySet<mathfu::vec4>(&args, kZValue)) {
    return;
  } else if (TrySet<mathfu::vec4i>(&args, kZValue)) {
    return;
  } else if (TrySet<mathfu::quat>(&args, kZValue)) {
    return;
  } else {
    frame->Error("set-z: invalid arguments");
  }
}

void SetW(ScriptFrame* frame) {
  Args args(frame);
  if (TrySet<mathfu::vec4>(&args, kWValue)) {
    return;
  } else if (TrySet<mathfu::vec4i>(&args, kWValue)) {
    return;
  } else if (TrySet<mathfu::quat>(&args, kWValue)) {
    return;
  } else {
    frame->Error("set-w: invalid arguments");
  }
}

LULLABY_SCRIPT_FUNCTION(Vec2Create, "vec2");
LULLABY_SCRIPT_FUNCTION(Vec3Create, "vec3");
LULLABY_SCRIPT_FUNCTION(Vec4Create, "vec4");
LULLABY_SCRIPT_FUNCTION(Vec2iCreate, "vec2i");
LULLABY_SCRIPT_FUNCTION(Vec3iCreate, "vec3i");
LULLABY_SCRIPT_FUNCTION(Vec4iCreate, "vec4i");
LULLABY_SCRIPT_FUNCTION(QuatCreate, "quat");
LULLABY_SCRIPT_FUNCTION(GetX, "get-x");
LULLABY_SCRIPT_FUNCTION(GetY, "get-y");
LULLABY_SCRIPT_FUNCTION(GetZ, "get-z");
LULLABY_SCRIPT_FUNCTION(GetW, "get-w");
LULLABY_SCRIPT_FUNCTION(SetX, "set-x");
LULLABY_SCRIPT_FUNCTION(SetY, "set-y");
LULLABY_SCRIPT_FUNCTION(SetZ, "set-z");
LULLABY_SCRIPT_FUNCTION(SetW, "set-w");

}  // namespace
}  // namespace lull
