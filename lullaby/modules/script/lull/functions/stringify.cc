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

#include <stdarg.h>
#include "lullaby/modules/script/lull/script_env.h"
#include "lullaby/modules/script/lull/script_frame.h"
#include "lullaby/modules/script/lull/script_types.h"

namespace lull {

std::string Stringify(const ScriptValue& value) {
  if (value.IsNil()) {
    return "nil";
  } else if (auto val = value.Get<bool>()) {
    return *val ? "true" : "false";
  } else if (auto val = value.Get<int>()) {
    return std::to_string(*val);
  } else if (auto val = value.Get<float>()) {
    return std::to_string(*val);
  } else if (auto val = value.Get<Symbol>()) {
    return std::to_string(val->value);
  } else if (auto val = value.Get<std::string>()) {
    return *val;
  } else if (auto val = value.Get<mathfu::vec2>()) {
    return std::to_string(val->x) + "," + std::to_string(val->y);
  } else if (auto val = value.Get<mathfu::vec3>()) {
    return std::to_string(val->x) + ", " + std::to_string(val->y) + ", " +
           std::to_string(val->z);
  } else if (auto val = value.Get<mathfu::vec4>()) {
    return std::to_string(val->x) + ", " + std::to_string(val->y) + ", " +
           std::to_string(val->z) + ", " + std::to_string(val->w);
  } else if (auto val = value.Get<Lambda>()) {
    return "[lambda]";
  } else if (auto val = value.Get<Macro>()) {
    return "[macro]";
  } else if (auto val = value.Get<NativeFunction>()) {
    return "[native func]";
  } else if (auto val = value.Get<TypedPointer>()) {
    return "[native ptr]";
  } else if (auto val = value.Get<AstNode>()) {
    return "[node]";
  } else {
    return "[unknown]";
  }
}

std::string Stringify(ScriptFrame* frame) {
  std::stringstream ss;

  while (frame->HasNext()) {
    ScriptValue value = frame->Next();
    frame->Return(value);

    if (auto val = value.Get<Symbol>()) {
      const ScriptValue value = frame->GetEnv()->GetValue(val->value);
      ScriptFrame f(frame->GetEnv(), value);
      ss << val->value << "@" << Stringify(&f);
    } else if (auto val = value.Get<AstNode>()) {
      auto child = val->first.Get<AstNode>();
      if (child) {
        ScriptFrame f1(frame->GetEnv(), val->first);
        ss << "( " << Stringify(&f1) << " )";
      } else {
        ss << Stringify(val->first);
      }

    } else {
      ss << Stringify(value);
    }

    if (frame->HasNext()) {
      ss << " ";
    }
  }
  return ss.str();
}

}  // namespace lull
