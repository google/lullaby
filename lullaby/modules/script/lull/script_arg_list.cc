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

#include "lullaby/modules/script/lull/script_arg_list.h"
#include "lullaby/modules/script/lull/script_env.h"

namespace lull {

ScriptArgList::ScriptArgList(ScriptEnv* env, ScriptValue args)
    : env_(env), args_(std::move(args)) {}

bool ScriptArgList::HasNext() const { return !args_.IsNil(); }

ScriptValue ScriptArgList::EvalNext() { return env_->Eval(Next()); }

ScriptValue ScriptArgList::Next() {
  ScriptValue next;
  if (args_.IsNil()) {
    env_->Error("No more arguments.", args_);
  } else if (const AstNode* node = args_.Get<AstNode>()) {
    next = args_;
    args_ = node->rest;
  } else {
    next = args_;
    args_ = ScriptValue();
  }
  return next;
}

}  // namespace lull
