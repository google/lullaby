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
#include "lullaby/util/clock.h"

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

namespace lull {
namespace {

// The following structs (prefixed with Is-) are used to categorize types with
// specific traits.  The details for how these are used and why they are
// necessary are explained further below.

template <typename T>
struct IsDuration {
  static constexpr bool value = std::is_same<T, Clock::duration>::value;
};

template <typename T>
struct IsMathfu {
  static constexpr bool value = std::is_same<T, mathfu::vec2>::value ||
                                std::is_same<T, mathfu::vec3>::value ||
                                std::is_same<T, mathfu::vec4>::value ||
                                std::is_same<T, mathfu::vec2i>::value ||
                                std::is_same<T, mathfu::vec3i>::value ||
                                std::is_same<T, mathfu::vec4i>::value ||
                                std::is_same<T, mathfu::quat>::value;
};

template <typename T>
struct IsLargeNum {
  static constexpr bool value =
      std::is_same<T, double>::value || std::is_same<T, int32_t>::value ||
      std::is_same<T, int64_t>::value || std::is_same<T, uint32_t>::value ||
      std::is_same<T, uint64_t>::value;
};

template <typename T, typename U>
struct IsBadDurationCombo {
  static constexpr bool value =
      std::is_same<T, Clock::duration>::value &&
      (IsMathfu<T>::value || std::is_same<U, Clock::duration>::value ||
       std::is_same<U, double>::value || std::is_same<U, float>::value ||
       std::is_same<U, uint64_t>::value);
};

template <typename T, typename U>
struct IsSignedAndUnsigned {
  static constexpr bool value = std::is_integral<T>::value &&
                                std::is_signed<T>::value &&
                                std::is_unsigned<U>::value;
};

// Certain numeric operations result in warnings (which are subsequently treated
// as errors) because of type mismatches.  For example multiplying an int with
// a float results in a "loss of precision" warning.  However, we want to ignore
// these warnings in LullScript, so we implicitly cast to the correct numeric
// type before performing the operation.  So, with an int*float operation, we
// would cast the int to a float first.  We only want to perform this conversion
// on numeric values.
template <typename T, typename U>
struct AreNumeric {
  static constexpr bool value =
      std::is_arithmetic<T>::value && !std::is_same<T, bool>::value &&
      std::is_arithmetic<U>::value && !std::is_same<U, bool>::value;
};

// Casts the |value| from U to T if both T and U are numeric types and
// implicitly convertible.  If they are not, then we simply return the |value|
// back as its original type.
template <typename T, typename U>
auto CastIfNumeric(const U& value) ->
    typename std::enable_if<AreNumeric<T, U>::value, T>::type {
  return static_cast<T>(value);
}

template <typename T, typename U>
auto CastIfNumeric(const U& value) ->
    typename std::enable_if<!AreNumeric<T, U>::value, const U&>::type {
  return value;
}

// The followings structs (prefixed with Enable-) are used to prevent the
// compiler from attempting to apply an operator on two types.

template <typename T, typename U>
struct EnableAddSub {
  static constexpr bool kIsDurationAndMathfu =
      IsDuration<T>::value && IsMathfu<U>::value;
  static constexpr bool kIsMathfuAndDuration =
      IsMathfu<T>::value && IsDuration<U>::value;
  static constexpr bool kIsLargeNumAndMathfu =
      IsLargeNum<T>::value && IsMathfu<U>::value;
  static constexpr bool kIsMathfuAndLargeNum =
      IsMathfu<T>::value && IsLargeNum<U>::value;
  // Addition and subtraction involving a double/[u]int[32/64]_t and a
  // vec2/vec3/vec4/quat causes a precision loss warning because the number is
  // implicitly cast to a float, so disable that case. Also, SFINAE seems to
  // fail for Clock::duration vs mathfu on msvc and it causes a build error
  // rather than just defaulting to the null implementation, so explicitly
  // disable this case as well.
  static constexpr bool value = !kIsDurationAndMathfu &&
                                !kIsMathfuAndDuration &&
                                !kIsLargeNumAndMathfu && !kIsMathfuAndLargeNum;
};

template <typename T, typename U>
struct EnableMulDivMod {
  // Multiplication, division, and modulo have the same conditions as other
  // arithmetic, except that MSVC builds fail for multiplying 2
  // Clock::durations, rather than just using SFINAE to disable this case, so
  // explicitly disable that case. We do that here instead of in EnableAddSub,
  // because adding 2 durations is valid. Also, Clock::duration vs
  // double/float/uint64_t results in a type with no TypeId.
  static constexpr bool value = EnableAddSub<T, U>::value &&
                                !IsBadDurationCombo<T, U>::value &&
                                !IsBadDurationCombo<U, T>::value;
};

template <typename T, typename U>
struct EnableComp {
  // Comparison of signed vs unsigned ints causes a warning, so disable it.
  static constexpr bool value =
      !IsSignedAndUnsigned<T, U>::value && !IsSignedAndUnsigned<U, T>::value;
};

// Macro tuple listing all types that can be operated on.
#define OPERABLE_TYPES         \
  OPERABLE_TYPE(int8_t)        \
  OPERABLE_TYPE(uint8_t)       \
  OPERABLE_TYPE(int16_t)       \
  OPERABLE_TYPE(uint16_t)      \
  OPERABLE_TYPE(int32_t)       \
  OPERABLE_TYPE(uint32_t)      \
  OPERABLE_TYPE(int64_t)       \
  OPERABLE_TYPE(uint64_t)      \
  OPERABLE_TYPE(float)         \
  OPERABLE_TYPE(double)        \
  OPERABLE_TYPE(mathfu::vec2)  \
  OPERABLE_TYPE(mathfu::vec3)  \
  OPERABLE_TYPE(mathfu::vec4)  \
  OPERABLE_TYPE(mathfu::vec2i) \
  OPERABLE_TYPE(mathfu::vec3i) \
  OPERABLE_TYPE(mathfu::vec4i) \
  OPERABLE_TYPE(mathfu::quat)  \
  OPERABLE_TYPE(Clock::duration)

// The ExpandOperandsAndApply function takes two Variants and converts them
// into concrete, valid pointers by trying to get all possible variations
// of the OPERABLE_TYPES macro on the Variants. It then calls Op::Apply<T, U>
// using the two pointers.
template <typename Op, typename T>
Variant ExpandOperandsAndApplyHelper(const T* lhs, const Variant* rhs) {
#define OPERABLE_TYPE(U)                                 \
  if (rhs->GetTypeId() == GetTypeId<U>()) {              \
    return Op::template Apply<T, U>(lhs, rhs->Get<U>()); \
  }
  OPERABLE_TYPES
#undef OPERABLE_TYPE
  return Variant();
}

template <typename Op>
Variant ExpandOperandsAndApply(const Variant* lhs, const Variant* rhs) {
#define OPERABLE_TYPE(T)                                            \
  if (lhs->GetTypeId() == GetTypeId<T>()) {                         \
    return ExpandOperandsAndApplyHelper<Op, T>(lhs->Get<T>(), rhs); \
  }
  OPERABLE_TYPES
#undef OPERABLE_TYPE
  return Variant();
}

// There are three parts to this macro.
//
// 1. Defines a helper class that uses SFINAE to either correctly apply an
// operator on two arugments returning the result, or does nothing and returns
// an empty Variant.
//
// There are two parts to the SFINAE evaluation:
//   Enable<T, U>::value:
//       Uses the provided "Enable" class from the macro to determine if the
//       requested operator is valid. This is used to provide additional
//       constraints on the evaluation beyond the decltype evaluation above.
//
//   decltype(T() Op U())>::type:
//       The return type of the operator applied to the two arguments. If the
//       operator does not work with the given types, the template substitution
//       will fail.
//
// If the template substitution succeeds, the Apply function will return the
// result of the operation on the values (passed by pointers). If template
// subsitution fails, a fallback Apply function is used that returns an Empty
// Variant.
//
// 2. Defines a function with the given Name that calls ExpandOperandsAndApply
// using the helper class and two Variant arguments.
//
// 3. Registers the function Name with LullScript as the Cmd.
#define LULLABY_SCRIPT_OPERATOR_FUNCTION(Name, Enable, Op, Cmd)              \
  struct Name##Impl {                                                        \
    template <typename T, typename U>                                        \
    static auto ApplyEnabled(const T* lhs, const U* rhs)                     \
        -> decltype(T() Op U()) {                                            \
      using R = decltype(T() Op U());                                        \
      return CastIfNumeric<R>(*lhs) Op CastIfNumeric<R>(*rhs);               \
    }                                                                        \
                                                                             \
    static Variant ApplyEnabled(const void* lhs, const void* rhs) {          \
      return Variant();                                                      \
    }                                                                        \
                                                                             \
    template <typename T, typename U>                                        \
    static auto Apply(const T* lhs, const U* rhs) ->                         \
        typename std::enable_if<Enable<T, U>::value, Variant>::type {        \
      return ApplyEnabled(lhs, rhs);                                         \
    }                                                                        \
                                                                             \
    template <typename T, typename U>                                        \
    static Variant Apply(const void* lhs, const void* rhs) {                 \
      return Variant();                                                      \
    }                                                                        \
  };                                                                         \
                                                                             \
  Variant Name(ScriptFrame* frame, const Variant* lhs, const Variant* rhs) { \
    Variant v = ExpandOperandsAndApply<Name##Impl>(lhs, rhs);                \
    if (v.Empty()) {                                                         \
      frame->Error("Unsupported operands for " Cmd);                         \
    }                                                                        \
    return v;                                                                \
  }                                                                          \
                                                                             \
  LULLABY_SCRIPT_FUNCTION_WRAP(Name, Cmd)

LULLABY_SCRIPT_OPERATOR_FUNCTION(Add, EnableAddSub, +, "+");
LULLABY_SCRIPT_OPERATOR_FUNCTION(Sub, EnableAddSub, -, "-");
LULLABY_SCRIPT_OPERATOR_FUNCTION(Mul, EnableMulDivMod, *, "*");
LULLABY_SCRIPT_OPERATOR_FUNCTION(Div, EnableMulDivMod, /, "/");
LULLABY_SCRIPT_OPERATOR_FUNCTION(Mod, EnableMulDivMod, %, "%");
LULLABY_SCRIPT_OPERATOR_FUNCTION(Eq, EnableComp, ==, "==");
LULLABY_SCRIPT_OPERATOR_FUNCTION(Ne, EnableComp, !=, "!=");
LULLABY_SCRIPT_OPERATOR_FUNCTION(Lt, EnableComp, <, "<");
LULLABY_SCRIPT_OPERATOR_FUNCTION(Gt, EnableComp, >, ">");
LULLABY_SCRIPT_OPERATOR_FUNCTION(Le, EnableComp, <=, "<=");
LULLABY_SCRIPT_OPERATOR_FUNCTION(Ge, EnableComp, >=, ">=");

}  // namespace
}  // namespace lull
