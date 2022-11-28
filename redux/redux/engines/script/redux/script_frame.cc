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

#include "redux/engines/script/redux/script_frame.h"

#include "redux/engines/script/redux/script_env.h"

namespace redux {

ScriptFrame::ScriptFrame(ScriptEnv* env, const ScriptValue& args)
    : env_(env), args_(&args) {
  CHECK(env_) << "Must provide environment for call frame!";
}

bool ScriptFrame::HasNext() const { return args_ && !args_->IsNil(); }

ScriptValue ScriptFrame::EvalNext() { return env_->Eval(Next()); }

const ScriptValue& ScriptFrame::Next() {
  CHECK(HasNext()) << "Check HasNext() before calling Next()";

  const ScriptValue& xargs = *args_;
  if (xargs.IsNil()) {
    env_->Error("No more arguments.", *args_);
  } else if (const AstNode* node = xargs.Get<AstNode>()) {
    args_ = &node->rest;
  } else {
    args_ = nullptr;
  }
  return xargs;
}

void ScriptFrame::Error(const char* message) { env_->Error(message, *args_); }

}  // namespace redux
