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
#include "lullaby/util/string_view.h"

namespace lull {

class ScriptFrame;
class ScriptValue;

// Converts the provided ScriptValue into a string for debugging purposes.
std::string Stringify(const ScriptValue& value);

// Converts the script snippet contained in the execution frame into a string
// for debugging purposes.
std::string Stringify(ScriptFrame* frame);

// Linked-list class used to simplify registering "built-in" functions with a
// script env.  Static instances of these classes are bound to C++ functions
// forming a linked list that can then be processed during ScriptEnv creation.
struct ScriptFunctionEntry {
  using Fn = void (*)(ScriptFrame*);
  ScriptFunctionEntry(Fn fn, string_view name);

  Fn fn = nullptr;
  string_view name;
  ScriptFunctionEntry* next = nullptr;
};

#define LULLABY_SCRIPT_FUNCTION(f, n) \
  ScriptFunctionEntry ScriptFunction_##f(&f, n);

}  // namespace lull

#endif  // LULLABY_MODULES_SCRIPT_LULL_FUNCTIONS_FUNCTIONS_H_
