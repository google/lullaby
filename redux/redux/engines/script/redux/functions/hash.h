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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_HASH_H_
#define REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_HASH_H_

#include "redux/engines/script/redux/script_env.h"

// This file implements the following script functions:
//
// (hash [string])
//   Returns the hash value for the given string.
//
// Note that redux script parsing treats any symbol that is prefixed with a
// colon as a short-cut for hash. In other words, the following two are
// equivalent:
//    (:hello)
//    (hash `hello`)

namespace redux {
namespace script_functions {

inline void HashFn(ScriptFrame* frame) {
  ScriptValue arg = frame->EvalNext();
  if (const std::string* str = arg.Get<std::string>()) {
    frame->Return(redux::Hash(*str));
  } else {
    frame->Return(HashValue(0));
  }
}

}  // namespace script_functions
}  // namespace redux

#endif  // REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_HASH_H_
