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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_TYPES_H_
#define REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_TYPES_H_

#include <functional>

#include "redux/engines/script/redux/script_value.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/base/typeid.h"

namespace redux {

class ScriptFrame;

// Represents a node in an abstract syntax tree (AST).
//
// An AstNode consists of two values. If the node is an internal node, then
// |first| will be another AstNode that represents the "child" of the node.  If
// the node is a leaf node, then |first| contains an actual value type (eg.
// int, string, vec3, etc.) And |rest| is always the next sibling of the AstNode
// or nil if there are no more siblings.
struct AstNode {
  AstNode(ScriptValue first, ScriptValue rest)
      : first(std::move(first)), rest(std::move(rest)) {}

  ScriptValue first;
  ScriptValue rest;
};

// Symbol (or identifier) that generally refers to a value that is stored in the
// script stack table.
struct Symbol {
  explicit Symbol(HashValue id) : value(id) {}
  HashValue value;

  bool operator==(Symbol rhs) const { return value == rhs.value; }
};

// Represents a macro definition, consisting of a parameter list (represented as
// a "flat" AST) and a body (also an AST).
struct Macro {
  Macro(ScriptValue params, ScriptValue body)
      : params(std::move(params)), body(std::move(body)) {}

  ScriptValue params;
  ScriptValue body;
};

// Represents a function definition, consisting of a parameter list (represented
// as a "flat" AST) and a body (also an AST).
struct Lambda {
  Lambda(ScriptValue params, ScriptValue body)
      : params(std::move(params)), body(std::move(body)) {}

  ScriptValue params;
  ScriptValue body;
};

// A special type used to indicate the desire to return from a function early.
struct DefReturn {
  explicit DefReturn(ScriptValue v) : value(std::move(v)) {}

  ScriptValue value;
};

// A wrapper around a native C++ function that allows the ScriptEnv to call it
// like any other script function.
struct NativeFunction {
  using Fn = std::function<void(ScriptFrame*)>;

  explicit NativeFunction(Fn fn) : fn(std::move(fn)) {}
  Fn fn = nullptr;
};
}  // namespace redux

REDUX_SETUP_TYPEID(redux::AstNode);
REDUX_SETUP_TYPEID(redux::DefReturn);
REDUX_SETUP_TYPEID(redux::Lambda);
REDUX_SETUP_TYPEID(redux::Macro);
REDUX_SETUP_TYPEID(redux::Symbol);
REDUX_SETUP_TYPEID(redux::NativeFunction);

#endif  // REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_TYPES_H_
