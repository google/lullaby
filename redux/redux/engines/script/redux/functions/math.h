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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_MATH_H_
#define REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_MATH_H_

#include "redux/engines/script/redux/script_env.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"

// This file implements the following script functions:
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

namespace redux {
namespace script_functions {

template <int N, typename T>
void GetElement(ScriptFrame* frame, const T* obj) {
  frame->Return((*obj)[N]);
}

template <int N, typename T>
void SetElement(T* obj, const ScriptValue& value) {
  using U = typename std::decay<decltype(T::data[0])>::type;

  U scalar{};
  if (!value.GetAs(&scalar)) {
    CHECK(false) << "Unable to set element " << N;
  }
  (*obj)[N] = scalar;
}

template <int N>
void GetElementFn(ScriptFrame* frame) {
  ScriptValue v0 = frame->EvalNext();
  if (auto x = v0.Get<quat>()) {
    GetElement<N>(frame, x);
    return;
  }
  if (auto x = v0.Get<vec4>()) {
    GetElement<N>(frame, x);
    return;
  }
  if (auto x = v0.Get<vec4i>()) {
    GetElement<N>(frame, x);
    return;
  }
  if constexpr (N < 3) {
    if (auto x = v0.Get<vec3>()) {
      GetElement<N>(frame, x);
      return;
    }
    if (auto x = v0.Get<vec3i>()) {
      GetElement<N>(frame, x);
      return;
    }
  }
  if constexpr (N < 2) {
    if (auto x = v0.Get<vec2>()) {
      GetElement<N>(frame, x);
      return;
    }
    if (auto x = v0.Get<vec2i>()) {
      GetElement<N>(frame, x);
      return;
    }
  }
}

template <int N>
void SetElementFn(ScriptFrame* frame) {
  ScriptValue v0 = frame->EvalNext();
  ScriptValue v1 = frame->EvalNext();

  if (auto x = v0.Get<quat>()) {
    SetElement<N>(x, v1);
    return;
  }
  if (auto x = v0.Get<vec4>()) {
    SetElement<N>(x, v1);
    return;
  }
  if (auto x = v0.Get<vec4i>()) {
    SetElement<N>(x, v1);
    return;
  }
  if constexpr (N < 3) {
    if (auto x = v0.Get<vec3>()) {
      SetElement<N>(x, v1);
      return;
    }
    if (auto x = v0.Get<vec3i>()) {
      SetElement<N>(x, v1);
      return;
    }
  }
  if constexpr (N < 2) {
    if (auto x = v0.Get<vec2>()) {
      SetElement<N>(x, v1);
      return;
    }
    if (auto x = v0.Get<vec2i>()) {
      SetElement<N>(x, v1);
      return;
    }
  }
}

}  // namespace script_functions
}  // namespace redux

#endif  // REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_MATH_H_
