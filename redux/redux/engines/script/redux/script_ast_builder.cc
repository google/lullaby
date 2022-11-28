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

#include "redux/engines/script/redux/script_ast_builder.h"

#include "redux/modules/base/logging.h"

namespace redux {

ScriptAstBuilder::ScriptAstBuilder() { Push(); }

void ScriptAstBuilder::Process(TokenType type, const void* ptr,
                               std::string_view token) {
  switch (type) {
    case kPush: {
      Push();
      break;
    }
    case kPop: {
      Pop();
      break;
    }
    case kPushArray: {
      Push();
      Append(Symbol(ConstHash("make-array")), "make-array");
      break;
    }
    case kPopArray: {
      Pop();
      break;
    }
    case kPushMap: {
      Push();
      Append(Symbol(ConstHash("make-map")), "make-map");
      break;
    }
    case kPopMap: {
      Pop();
      break;
    }
    case kBool: {
      const auto value = *reinterpret_cast<const bool*>(ptr);
      Append(value, token);
      break;
    }
    case kInt8: {
      const auto value = *reinterpret_cast<const int8_t*>(ptr);
      Append(value, token);
      break;
    }
    case kUint8: {
      const auto value = *reinterpret_cast<const uint8_t*>(ptr);
      Append(value, token);
      break;
    }
    case kInt16: {
      const auto value = *reinterpret_cast<const int16_t*>(ptr);
      Append(value, token);
      break;
    }
    case kUint16: {
      const auto value = *reinterpret_cast<const uint16_t*>(ptr);
      Append(value, token);
      break;
    }
    case kInt32: {
      const auto value = *reinterpret_cast<const int32_t*>(ptr);
      Append(value, token);
      break;
    }
    case kUint32: {
      const auto value = *reinterpret_cast<const uint32_t*>(ptr);
      Append(value, token);
      break;
    }
    case kInt64: {
      const auto value = *reinterpret_cast<const int64_t*>(ptr);
      Append(value, token);
      break;
    }
    case kUint64: {
      const auto value = *reinterpret_cast<const uint64_t*>(ptr);
      Append(value, token);
      break;
    }
    case kFloat: {
      const auto value = *reinterpret_cast<const float*>(ptr);
      Append(value, token);
      break;
    }
    case kDouble: {
      const auto value = *reinterpret_cast<const double*>(ptr);
      Append(value, token);
      break;
    }
    case kHashValue: {
      const auto value = *reinterpret_cast<const HashValue*>(ptr);
      Append(value, token);
      break;
    }
    case kNull: {
      Append(Var(), token);
      break;
    }
    case kSymbol: {
      const auto value = *reinterpret_cast<const Symbol*>(ptr);
      Append(value, token);
      break;
    }
    case kString: {
      const std::string value(*reinterpret_cast<const std::string_view*>(ptr));
      Append(value, token);
      break;
    }
    case kEof: {
      break;
    }
  }
}

void ScriptAstBuilder::Append(Var value, std::string_view token) {
  ScriptValue first(std::move(value));
  ScriptValue node(AstNode(std::move(first), ScriptValue()));

  List& list = stack_.back();
  if (list.head.IsNil()) {
    // The current list being created is empty, so both the head and tail should
    // point to the newly created value.
    list.head = node;
    list.tail = node;
  } else {
    // Set the tail's sibling to the newly created value.
    list.tail.Get<AstNode>()->rest = node;
    // Update the tail to point to the newly created value.
    list.tail = node;
  }
}

void ScriptAstBuilder::Push() { stack_.emplace_back(); }

void ScriptAstBuilder::Pop() {
  List list = std::move(stack_.back());
  stack_.pop_back();

  if (Var* var = list.head.Get<Var>()) {
    Append(std::move(*var), "");
  } else {
    LOG(ERROR) << "There were errors trying to build the AST.";
  }
}

const AstNode* ScriptAstBuilder::GetRoot() const {
  if (error_) {
    return nullptr;
  }
  if (stack_.empty()) {
    return nullptr;
  }
  const List& list = stack_.back();
  if (list.head.IsNil()) {
    return nullptr;
  }
  return list.head.Get<AstNode>();
}

void ScriptAstBuilder::Error(std::string_view token, std::string_view message) {
  LOG(WARNING) << "Error parsing " << token << ": " << message;
  error_ = true;
}

}  // namespace redux
