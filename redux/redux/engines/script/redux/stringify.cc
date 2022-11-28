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

#include <sstream>

#include "redux/engines/script/redux/script_env.h"
#include "redux/engines/script/redux/script_frame.h"
#include "redux/engines/script/redux/script_types.h"
#include "redux/modules/base/typed_ptr.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"
#include "redux/modules/var/var_array.h"
#include "redux/modules/var/var_table.h"

namespace redux {

// Forward-declaration of Stringify so it can be used from helpers.
std::string Stringify(const ScriptValue& value);

/*
std::string StringifyVarArray(const VarArray& array) {
  std::stringstream ss;
  for (const auto& variant : array) {
    ss << "(" << Stringify(variant) << ")";
  }
  return ss.str();
}

std::string StringifyVarTable(const VarTable& map) {
  std::stringstream ss;
  for (const auto& kvp : map) {
    ss << "(" << Stringify(kvp.first) << ": "
       << Stringify(kvp.second) << ")";
  }

  return ss.str();
}
*/

std::string Stringify(const ScriptValue& value) {
  if (!value) {
    return "nil";
  } else if (value.IsNil()) {
    return "nil";
  } else if (auto val = value.Get<bool>()) {
    return *val ? "true" : "false";
  } else if (auto val = value.Get<int8_t>()) {
    return std::to_string(static_cast<int>(*val));
  } else if (auto val = value.Get<int16_t>()) {
    return std::to_string(*val);
  } else if (auto val = value.Get<int32_t>()) {
    return std::to_string(*val);
  } else if (auto val = value.Get<int64_t>()) {
    return std::to_string(*val);
  } else if (auto val = value.Get<uint8_t>()) {
    return std::to_string(static_cast<int>(*val)) + "u";
  } else if (auto val = value.Get<uint16_t>()) {
    return std::to_string(*val) + "u";
  } else if (auto val = value.Get<uint32_t>()) {
    return std::to_string(*val) + "u";
  } else if (auto val = value.Get<uint64_t>()) {
    return std::to_string(*val) + "u";
  } else if (auto val = value.Get<float>()) {
    return std::to_string(*val);
  } else if (auto val = value.Get<Symbol>()) {
    return std::to_string(val->value.get());
  } else if (auto val = value.Get<std::string>()) {
    return *val;
  } else if (auto val = value.Get<vec2>()) {
    return std::to_string(val->x) + "," + std::to_string(val->y);
  } else if (auto val = value.Get<vec3>()) {
    return std::to_string(val->x) + ", " + std::to_string(val->y) + ", " +
           std::to_string(val->z);
  } else if (auto val = value.Get<vec4>()) {
    return std::to_string(val->x) + ", " + std::to_string(val->y) + ", " +
           std::to_string(val->z) + ", " + std::to_string(val->w);
  } else if (auto val = value.Get<quat>()) {
    return std::to_string(val->x) + ", " + std::to_string(val->y) + ", " +
           std::to_string(val->z) + ", " + std::to_string(val->w);
  } else if (auto val = value.Get<vec2i>()) {
    return std::to_string(val->x) + "," + std::to_string(val->y);
  } else if (auto val = value.Get<vec3i>()) {
    return std::to_string(val->x) + ", " + std::to_string(val->y) + ", " +
           std::to_string(val->z);
  } else if (auto val = value.Get<vec4i>()) {
    return std::to_string(val->x) + ", " + std::to_string(val->y) + ", " +
           std::to_string(val->z) + ", " + std::to_string(val->w);
    // } else if (auto val = value.Get<Entity>()) {
    //   return "[Entity " + to_string(*val) + "]";
    // } else if (auto val = value.Get<VarArray>()) {
    //   return "[array " + StringifyVarArray(*val) + "]";
    // } else if (auto val = value.Get<VarTable>()) {
    //   return "[map " + StringifyVarTable(*val) + "]";
    // } else if (auto val = value.Get<Message>()) {
    //   return "[event " + StringifyVar(val->GetTypeId()) + " " +
    //          StringifyVarTable(*val->GetValues()) + "]";
  } else if (auto val = value.Get<Lambda>()) {
    return "[lambda]";
  } else if (auto val = value.Get<Macro>()) {
    return "[macro]";
  } else if (auto val = value.Get<NativeFunction>()) {
    return "[native func]";
  } else if (auto val = value.Get<TypedPtr>()) {
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
    const ScriptValue& value = frame->Next();
    frame->Return(value);

    if (auto val = value.Get<Symbol>()) {
      ScriptValue value = frame->GetEnv()->GetValue(val->value);
      ScriptFrame f(frame->GetEnv(), value);
      ss << val->value.get() << "@" << Stringify(&f);
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

}  // namespace redux
