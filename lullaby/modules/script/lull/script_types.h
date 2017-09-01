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

#ifndef LULLABY_MODULES_SCRIPT_LULL_SCRIPT_TYPES_H_
#define LULLABY_MODULES_SCRIPT_LULL_SCRIPT_TYPES_H_

#include <functional>
#include <vector>
#include "lullaby/modules/script/lull/script_value.h"
#include "lullaby/util/typeid.h"

namespace lull {

class ScriptFrame;

using ScriptByteCode = std::vector<uint8_t>;

// Represents a node in an abstract syntax tree (AST).
//
// An AstNode consists of two ScriptValues.  (Remember that ScriptValues are
// basically an intrusive_ptr<Variant>).  If the node is an internal node, then
// |first| will be another AstNode that represents the "child" of the node.  If
// the node is a leaf node, then |first| contains an actual value type (eg.
// int, string, vec3, etc.) And |rest| is always the next sibling of the AstNode
// or nil if there are no more siblings.
struct AstNode {
  AstNode() {}
  AstNode(ScriptValue first, ScriptValue rest)
      : first(std::move(first)), rest(std::move(rest)) {}
  ScriptValue first;
  ScriptValue rest;
};

// Reprents a Symbol (or identifier) in a parsed script.  Symbols refer to a
// value that is stored in the symbol table.
struct Symbol {
  Symbol() {}
  explicit Symbol(HashValue value) : value(value) {}
  HashValue value = 0;
};

// A ScriptValue type that represents a macro in the script.  It consists of
// a parameter list (represented as a "flat" AST) and a body (also an AST).
struct Macro {
  Macro(ScriptValue params, ScriptValue body)
      : params(std::move(params)), body(std::move(body)) {}
  ScriptValue params;
  ScriptValue body;
};

// A ScriptValue type that represents a function in the script.  It consists of
// a parameter list (represented as a "flat" AST) and a function body (also an
// AST).
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

// A wrapper around a native C++ function that can be stored as a ScriptValue.
// This function can then be called like any other script function.  See
// ScriptFrame for more information.
struct NativeFunction {
  using Fn = std::function<void(ScriptFrame*)>;
  explicit NativeFunction(Fn fn) : fn(std::move(fn)) {}
  Fn fn = nullptr;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::DefReturn);
LULLABY_SETUP_TYPEID(lull::AstNode);
LULLABY_SETUP_TYPEID(lull::Lambda);
LULLABY_SETUP_TYPEID(lull::Macro);
LULLABY_SETUP_TYPEID(lull::Symbol);
LULLABY_SETUP_TYPEID(lull::NativeFunction);

#endif  // LULLABY_MODULES_SCRIPT_LULL_SCRIPT_TYPES_H_
