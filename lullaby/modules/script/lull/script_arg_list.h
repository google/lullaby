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

#ifndef LULLABY_MODULES_SCRIPT_LULL_SCRIPT_ARG_LIST_H_
#define LULLABY_MODULES_SCRIPT_LULL_SCRIPT_ARG_LIST_H_

#include "lullaby/modules/script/lull/script_value.h"

namespace lull {

class ScriptEnv;

// Represents a list of ScriptValue arguments.
//
// Individual arguments can be "popped" off the list by calling Next() or
// EvalNext().  Next() simply returns the next ScriptValue in the arglist,
// whereas EvalNext() returns the evaluated result of the next ScriptValue.  The
// difference between using Next() and EvalNext() is effectively the difference
// between how a function and a macro are called.
class ScriptArgList {
 public:
  // Constructs the arglist.
  ScriptArgList(ScriptEnv* env, ScriptValue args);

  // Returns true if there is another argument in the list.
  bool HasNext() const;

  // Returns the next argument.
  ScriptValue Next();

  // Evaluates the next argument and returns its result.
  ScriptValue EvalNext();

 protected:
  ScriptEnv* env_ = nullptr;
  ScriptValue args_;
};

}  // namespace lull

#endif  // LULLABY_MODULES_SCRIPT_LULL_SCRIPT_ARG_LIST_H_
