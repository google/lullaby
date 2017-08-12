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

#include "lullaby/modules/script/lull/script_ast_builder.h"
#include "lullaby/modules/script/lull/script_env.h"

namespace lull {

ScriptAstBuilder::ScriptAstBuilder(ScriptEnv* env) : env_(env) { Push(); }

void ScriptAstBuilder::Process(CodeType type, const void* ptr,
                               string_view token) {
  switch (type) {
    case kNil: {
      Append(ScriptValue(), token);
      break;
    }
    case kPush: {
      Push();
      break;
    }
    case kPop: {
      Pop();
      break;
    }
    case kBool: {
      const auto value = env_->Create(*reinterpret_cast<const bool*>(ptr));
      Append(value, token);
      break;
    }
    case kInt: {
      const auto value = env_->Create(*reinterpret_cast<const int*>(ptr));
      Append(value, token);
      break;
    }
    case kFloat: {
      const auto value = env_->Create(*reinterpret_cast<const float*>(ptr));
      Append(value, token);
      break;
    }
    case kSymbol: {
      const auto value = env_->Create(*reinterpret_cast<const Symbol*>(ptr));
      Append(value, token);
      break;
    }
    case kString: {
      const auto str = reinterpret_cast<const string_view*>(ptr)->to_string();
      const auto value = env_->Create(str);
      Append(value, token);
      break;
    }
    case kEof: {
      break;
    }
  }
}

void ScriptAstBuilder::Append(const ScriptValue& value, string_view token) {
  ScriptValue node = env_->Create(AstNode(value, ScriptValue()));

  ScriptValueList& list = stack_.back();
  if (list.head.IsNil() && list.tail.IsNil()) {
    // The current list being created is empty, so both the head and tail should
    // point to the newly created ScriptValue.
    list.head = node;
    list.tail = node;
  } else {
    // Set the tail's sibling to the newly created ScriptValue.
    list.tail.Get<AstNode>()->rest = node;
    // Update the tail to point to the newly created ScriptValue.
    list.tail = node;
  }
}

void ScriptAstBuilder::Push() { stack_.emplace_back(); }

void ScriptAstBuilder::Pop() {
  ScriptValueList list = std::move(stack_.back());
  stack_.pop_back();
  Append(list.head, "");
}

AstNode ScriptAstBuilder::GetRoot() const {
  if (error_) {
    return AstNode();
  }
  if (stack_.empty()) {
    return AstNode();
  }
  const ScriptValueList& list = stack_.back();
  if (list.head.IsNil()) {
    return AstNode();
  }
  const AstNode* node = list.head.Get<AstNode>();
  return node ? *node : AstNode();
}

void ScriptAstBuilder::Error(string_view token, string_view message) {
  LOG(WARNING) << "Error parsing " << token.to_string() << ": " <<
      message.to_string();
  error_ = true;
}

}  // namespace lull
