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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_OPERATORS_H_
#define REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_OPERATORS_H_

#include "redux/engines/script/redux/script_env.h"
#include "redux/modules/ecs/entity.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"

// This file implements the following script functions:
//
// (== [lhs] [rhs])
//   Returns true if two arguments have the same value. Valid for: integers,
//   floating-point numbers, Clock::duration, and mathfu types.
//
// (!= [lhs] [rhs])
//   Returns true if two arguments have different values. Valid for: integers,
//   floating-point numbers, Clock::duration, and mathfu types.
//
// (< [lhs] [rhs])
//   Returns true if first argument's value is less than the second argument's
//   value. Valid for: integers, floating-point numbers, and Clock::duration.
//
// (> [lhs] [rhs])
//   Returns true if first argument's value is greater than the second
//   argument's value. Valid for: integers, floating-point numbers, and
//   Clock::duration.
//
// (<= [lhs] [rhs])
//   Returns true if first argument's value is less than or equal to the second
//   argument's value. Valid for: integers, floating-point numbers, and
//   Clock::duration.
//
// (>= [lhs] [rhs])
//   Returns true if first argument's value is greater than or equal to the
//   second argument's value. Valid for: integers, floating-point numbers, and
//   Clock::duration.
//
// (+ [value] [value])
//   Returns the sum of the arguments.
//
// (- [value] [value])
//   Returns the difference of the arguments.
//
// (* [value] [value])
//   Returns the multiplicative product of the arguments.
//
// (/ [value] [value])
//   Returns the divided quotient of the arguments.
//
// (% [value] [value])
//   Returns the modulo of the arguments.
//
// (and [args...])
//   Returns false if any of the arguments is false, true otherwise.
//
// (or [args...])
//   Returns true if any of the arguments is true, false otherwise.
//
// (not [arg])
//   Returns false if arg is true or true if arg is false.

namespace redux {
namespace script_functions {

// For each operator, we want:
//
// - A template function for the operator that returns an std::false_type. This
//   declaration is used as a fallback if an actual operator isn't defined that
//   operates on the two types.
//
// - A Traits class that has a single "kEnabled" constexpr bool value that we
//   use to decide if we actually want to support this operator for the given
//   types. The default implementation for this class uses the return type of
//   the result of applying the operator to both types.  If the return type is
//   std::false_type, it means we could only "find" the template function we
//   defined earlier. If we get a different return type, it means that an actual
//   operator is defined somewhere for these two arguments.
//
// - A ScriptOp class with an operator() that applies the operator to the args
//   and stores it into the ScriptFrame. By "default", this will be a no-op
//   (with a CHECK-fail). However, if our Traits class above declares that
//   the function is valid for the given types, then we'll call the operator
//   directly and store the result in the ScriptFrame.

#define SCRIPT_OP(NAME__, OP__)                                             \
  template <typename T, typename U>                                         \
  std::false_type operator OP__(T, U);                                      \
                                                                            \
  template <typename T, typename U>                                         \
  struct NAME__##Traits {                                                   \
    using Res = decltype(std::declval<T>() OP__ std::declval<U>());         \
    static constexpr bool kEnabled = !std::is_same_v<Res, std::false_type>; \
  };                                                                        \
                                                                            \
  struct ScriptOp##NAME__ {                                                 \
    void operator()(ScriptFrame*, const void*, const void*) const {         \
      CHECK(false);                                                         \
    }                                                                       \
                                                                            \
    template <typename T, typename U>                                       \
    typename std::enable_if<NAME__##Traits<T, U>::kEnabled, void>::type     \
    operator()(ScriptFrame* frame, const T* lhs, const U* rhs) const {      \
      frame->Return(*lhs OP__ *rhs);                                        \
    }                                                                       \
  };

// Define the above templates for all the binary operators we want to support.
SCRIPT_OP(Eq, ==);
SCRIPT_OP(Ne, !=);
SCRIPT_OP(Lt, <);
SCRIPT_OP(Le, <=);
SCRIPT_OP(Gt, >);
SCRIPT_OP(Ge, >=);
SCRIPT_OP(Add, +);
SCRIPT_OP(Sub, -);
SCRIPT_OP(Mul, *);
SCRIPT_OP(Div, /);
SCRIPT_OP(Mod, %);

// There are cases (mostly dealing with templates) where we have a declaration
// that "matches" what we're looking for, but we can't actually call that
// operator. In such cases, we can explicitly "disable" the lookup.
struct DisableOp {
  static constexpr bool kEnabled = false;
};
template <>
struct MulTraits<quat, vec2> : DisableOp {};
template <>
struct MulTraits<quat, vec2i> : DisableOp {};
template <>
struct MulTraits<quat, vec3i> : DisableOp {};
template <>
struct MulTraits<quat, vec4> : DisableOp {};
template <>
struct MulTraits<quat, vec4i> : DisableOp {};

template <>
struct MulTraits<vec2i, absl::Duration> : DisableOp {};
template <>
struct MulTraits<vec3i, absl::Duration> : DisableOp {};
template <>
struct MulTraits<vec4i, absl::Duration> : DisableOp {};
template <>
struct MulTraits<vec2, absl::Duration> : DisableOp {};
template <>
struct MulTraits<vec3, absl::Duration> : DisableOp {};
template <>
struct MulTraits<vec4, absl::Duration> : DisableOp {};
template <>
struct MulTraits<quat, absl::Duration> : DisableOp {};
template <>
struct MulTraits<Entity, absl::Duration> : DisableOp {};
template <>
struct MulTraits<absl::Duration, vec2i> : DisableOp {};
template <>
struct MulTraits<absl::Duration, vec3i> : DisableOp {};
template <>
struct MulTraits<absl::Duration, vec4i> : DisableOp {};
template <>
struct MulTraits<absl::Duration, vec2> : DisableOp {};
template <>
struct MulTraits<absl::Duration, vec3> : DisableOp {};
template <>
struct MulTraits<absl::Duration, vec4> : DisableOp {};
template <>
struct MulTraits<absl::Duration, quat> : DisableOp {};
template <>
struct MulTraits<absl::Duration, Entity> : DisableOp {};
template <>
struct MulTraits<absl::Duration, absl::Duration> : DisableOp {};

template <>
struct DivTraits<vec2i, absl::Duration> : DisableOp {};
template <>
struct DivTraits<vec3i, absl::Duration> : DisableOp {};
template <>
struct DivTraits<vec4i, absl::Duration> : DisableOp {};
template <>
struct DivTraits<vec2, absl::Duration> : DisableOp {};
template <>
struct DivTraits<vec3, absl::Duration> : DisableOp {};
template <>
struct DivTraits<vec4, absl::Duration> : DisableOp {};
template <>
struct DivTraits<quat, absl::Duration> : DisableOp {};
template <>
struct DivTraits<Entity, absl::Duration> : DisableOp {};
template <>
struct DivTraits<absl::Duration, vec2i> : DisableOp {};
template <>
struct DivTraits<absl::Duration, vec3i> : DisableOp {};
template <>
struct DivTraits<absl::Duration, vec4i> : DisableOp {};
template <>
struct DivTraits<absl::Duration, vec2> : DisableOp {};
template <>
struct DivTraits<absl::Duration, vec3> : DisableOp {};
template <>
struct DivTraits<absl::Duration, vec4> : DisableOp {};
template <>
struct DivTraits<absl::Duration, quat> : DisableOp {};
template <>
struct DivTraits<absl::Duration, Entity> : DisableOp {};
template <>
struct DivTraits<absl::Duration, absl::Duration> : DisableOp {};

template <>
struct ModTraits<int, float> : DisableOp {};
template <>
struct ModTraits<int, double> : DisableOp {};
template <>
struct ModTraits<int8_t, float> : DisableOp {};
template <>
struct ModTraits<int8_t, double> : DisableOp {};
template <>
struct ModTraits<uint8_t, float> : DisableOp {};
template <>
struct ModTraits<uint8_t, double> : DisableOp {};
template <>
struct ModTraits<int16_t, float> : DisableOp {};
template <>
struct ModTraits<int16_t, double> : DisableOp {};
template <>
struct ModTraits<uint16_t, float> : DisableOp {};
template <>
struct ModTraits<uint16_t, double> : DisableOp {};
template <>
struct ModTraits<int64_t, float> : DisableOp {};
template <>
struct ModTraits<int64_t, double> : DisableOp {};
template <>
struct ModTraits<uint64_t, float> : DisableOp {};
template <>
struct ModTraits<uint64_t, double> : DisableOp {};
template <>
struct ModTraits<unsigned int, float> : DisableOp {};
template <>
struct ModTraits<unsigned int, double> : DisableOp {};

template <>
struct ModTraits<float, int> : DisableOp {};
template <>
struct ModTraits<double, int> : DisableOp {};
template <>
struct ModTraits<float, int8_t> : DisableOp {};
template <>
struct ModTraits<double, int8_t> : DisableOp {};
template <>
struct ModTraits<float, uint8_t> : DisableOp {};
template <>
struct ModTraits<double, uint8_t> : DisableOp {};
template <>
struct ModTraits<float, int16_t> : DisableOp {};
template <>
struct ModTraits<double, int16_t> : DisableOp {};
template <>
struct ModTraits<float, uint16_t> : DisableOp {};
template <>
struct ModTraits<double, uint16_t> : DisableOp {};
template <>
struct ModTraits<float, int64_t> : DisableOp {};
template <>
struct ModTraits<double, int64_t> : DisableOp {};
template <>
struct ModTraits<float, uint64_t> : DisableOp {};
template <>
struct ModTraits<double, uint64_t> : DisableOp {};

template <>
struct ModTraits<float, unsigned int> : DisableOp {};
template <>
struct ModTraits<double, unsigned int> : DisableOp {};
template <>
struct ModTraits<float, float> : DisableOp {};
template <>
struct ModTraits<float, double> : DisableOp {};
template <>
struct ModTraits<double, float> : DisableOp {};
template <>
struct ModTraits<double, double> : DisableOp {};

// All the different types against which we want to be able to apply a binary
// operator.
#define EXPAND_TYPES \
  TYPE(int)          \
  TYPE(unsigned int) \
  TYPE(float)        \
  TYPE(double)       \
  TYPE(int8_t)       \
  TYPE(uint8_t)      \
  TYPE(int16_t)      \
  TYPE(uint16_t)     \
  TYPE(int64_t)      \
  TYPE(uint64_t)     \
  TYPE(vec2i)        \
  TYPE(vec3i)        \
  TYPE(vec4i)        \
  TYPE(vec2)         \
  TYPE(vec3)         \
  TYPE(vec4)         \
  TYPE(quat)         \
  TYPE(Entity)       \
  TYPE(absl::Duration)

template <typename Op, typename T>
void ExpandRhs(ScriptFrame* frame, const T* lhs, const ScriptValue& rhs) {
#define TYPE(T__)                \
  if (auto x = rhs.Get<T__>()) { \
    Op{}(frame, lhs, x);         \
    return;                      \
  }
  EXPAND_TYPES
#undef TYPE
  CHECK(false);
}

template <typename Op>
void ExpandLhs(ScriptFrame* frame, const ScriptValue& lhs,
               const ScriptValue& rhs) {
#define TYPE(T__)                 \
  if (auto x = lhs.Get<T__>()) {  \
    ExpandRhs<Op>(frame, x, rhs); \
    return;                       \
  }
  EXPAND_TYPES
#undef TYPE
  CHECK(false) << lhs.GetTypeId() << " " << rhs.GetTypeId();
}

template <typename Op>
void ApplyOperator(ScriptFrame* frame) {
  const ScriptValue a1 = frame->EvalNext();
  const ScriptValue a2 = frame->EvalNext();
  ExpandLhs<Op>(frame, a1, a2);
}

inline void EqFn(ScriptFrame* frame) { ApplyOperator<ScriptOpEq>(frame); }

inline void NeFn(ScriptFrame* frame) { ApplyOperator<ScriptOpNe>(frame); }

inline void LtFn(ScriptFrame* frame) { ApplyOperator<ScriptOpLt>(frame); }

inline void LeFn(ScriptFrame* frame) { ApplyOperator<ScriptOpLe>(frame); }

inline void GtFn(ScriptFrame* frame) { ApplyOperator<ScriptOpGt>(frame); }

inline void GeFn(ScriptFrame* frame) { ApplyOperator<ScriptOpGe>(frame); }

inline void AddFn(ScriptFrame* frame) { ApplyOperator<ScriptOpAdd>(frame); }

inline void SubFn(ScriptFrame* frame) { ApplyOperator<ScriptOpSub>(frame); }

inline void MulFn(ScriptFrame* frame) { ApplyOperator<ScriptOpMul>(frame); }

inline void DivFn(ScriptFrame* frame) { ApplyOperator<ScriptOpDiv>(frame); }

inline void ModFn(ScriptFrame* frame) { ApplyOperator<ScriptOpMod>(frame); }

inline void AndFn(ScriptFrame* frame) {
  if (!frame->HasNext()) {
    frame->Return(false);
    return;
  }

  bool result = true;
  while (result && frame->HasNext()) {
    ScriptValue arg = frame->EvalNext();
    if (!arg.Is<bool>()) {
      frame->Error("and: argument should have type bool.");
      return;
    }
    result &= *arg.Get<bool>();
  }
  frame->Return(result);
}

inline void OrFn(ScriptFrame* frame) {
  if (!frame->HasNext()) {
    frame->Return(false);
    return;
  }

  bool result = false;
  while (!result && frame->HasNext()) {
    ScriptValue arg = frame->EvalNext();
    if (!arg.Is<bool>()) {
      frame->Error("or: argument should have type bool.");
      return;
    }
    result |= *arg.Get<bool>();
  }
  frame->Return(result);
}

inline bool NotFn(bool arg) { return !arg; }

}  // namespace script_functions
}  // namespace redux

#endif  // REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_OPERATORS_H_
