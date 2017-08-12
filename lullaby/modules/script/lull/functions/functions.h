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

#ifndef LULLABY_MODULES_SCRIPT_LULL_FUNCTIONS_FUNCTIONS_H_
#define LULLABY_MODULES_SCRIPT_LULL_FUNCTIONS_FUNCTIONS_H_

#include <string>

namespace lull {

class ScriptFrame;
class ScriptValue;

// This file contains the declarations for the various "built-in" functions for
// LullScript.  These functions extract arguments from a ScriptFrame (either
// with evaluation or without), and set the "return value" into the ScriptFrame.

// Evaluates a sequence of statements, returning the result of the last one.
// (do [statement] [statement] [statement] ...)
void Do(ScriptFrame* frame);

// Conditionally evaluates one of two statements based on a boolean condition.
// (do [condition] [true-statement] [false-statement])
void Cond(ScriptFrame* frame);

// Returns the sum of all arguments.
// (+ [value] [value] ...)
void Add(ScriptFrame* frame);

// Returns the difference of all arguments.
// (- [value] [value] ...)
void Subtract(ScriptFrame* frame);

// Returns the product of all arguments.
// (* [value] [value] ...)
void Multiply(ScriptFrame* frame);

// Returns the quotient of all arguments.
// (/ [value] [value] ...)
void Divide(ScriptFrame* frame);

// Returns the modulo of two arguments.
// (% [value] [value])
void Modulo(ScriptFrame* frame);

// Returns true if two arguments have the same value.
// (== [lhs] [rhs])
void Equal(ScriptFrame* frame);

// Returns true if two arguments do not have the same value.
// (!= [lhs] [rhs])
void NotEqual(ScriptFrame* frame);

// Returns true if first argument's value is less than the second argument's
// value.
// (< [lhs] [rhs])
void LessThan(ScriptFrame* frame);

// Returns true if first argument's value is greater than the second argument's
// value.
// (> [lhs] [rhs])
void GreaterThan(ScriptFrame* frame);

// Returns true if first argument's value is less than or equal to the second
// argument's value.
// (<= [lhs] [rhs])
void LessThanOrEqual(ScriptFrame* frame);

// Returns true if first argument's value is greater than or equal to the second
// argument's value.
// (>= [lhs] [rhs])
void GreaterThanOrEqual(ScriptFrame* frame);

// Returns a mathfu::vec2 by extracting values from the arguments.
// (vec2 [x] [y])
void CreateVec2(ScriptFrame* frame);

// Returns a mathfu::vec3 by extracting values from the arguments.
// (vec3 [x] [y] [z])
void CreateVec3(ScriptFrame* frame);

// Returns a mathfu::vec4 by extracting values from the arguments.
// (vec4 [x] [y] [z] [w])
void CreateVec4(ScriptFrame* frame);

// Returns a mathfu::quat by extracting values from the arguments.
// (quat [w] [x] [y] [z])
void CreateQuat(ScriptFrame* frame);

// Returns a std::unordered_map by extracting values from the arguments.  The
// keys must be HashValues.
// (make-map ([key] [value]) ([key] [value]) ...)
void CreateMap(ScriptFrame* frame);

// Converts the provided ScriptValue into a string for debugging purposes.
std::string Stringify(const ScriptValue& value);

// Converts the script snippet contained in the execution frame into a string
// for debugging purposes.
std::string Stringify(ScriptFrame* frame);

}  // namespace lull

#endif  // LULLABY_MODULES_SCRIPT_LULL_FUNCTIONS_FUNCTIONS_H_
