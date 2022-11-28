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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_MESSAGE_H_
#define REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_MESSAGE_H_

#include "redux/engines/script/redux/script_env.h"

// This file implements the following script functions:
//
// (make-event [type] [(map ...)] )
//   Creates an event wrapper with the optional map of values.  The type
//   must have integer or hashvalue type.
//
// (event-type [event])
//   Returns the name of the event.
//
// (event-size [event])
//   Returns the number of elements in the event.
//
// (event-get [event] [key])
//   Returns the value associated with the key in the event.  The key must be an
//   integer or hashvalue type.

#include "redux/modules/dispatcher/message.h"

namespace redux {
namespace script_functions {

inline void MessageCreateFn(ScriptFrame* frame) {
  TypeId type(0);
  if (frame->HasNext()) {
    ScriptValue type_arg = frame->EvalNext();
    uint32_t id = 0;
    type_arg.GetAs(&id);
    type = TypeId(id);
  }

  if (type == TypeId(0)) {
    frame->Error("msg: type not provided");
    return;
  }

  if (frame->HasNext()) {
    ScriptValue values_arg = frame->EvalNext();
    const VarTable* values = values_arg.Get<VarTable>();
    CHECK(values);
    frame->Return(Message(type, *values));
  } else {
    frame->Return(Message(type));
  }
}

inline TypeId MessageTypeFn(const Message* msg) { return msg->GetTypeId(); }

inline Var MessageGetOrFn(const Message* msg, HashValue key,
                          const Var* default_value) {
  const auto* values = msg->Get<VarTable>();
  if (!values) {
    return *default_value;
  }
  const Var* var = values->TryFind(key);
  if (var) {
    return *var;
  }
  return *default_value;
}

inline Var MessageGetFn(const Message* msg, HashValue key) {
  Var nil;
  return MessageGetOrFn(msg, key, &nil);
}
}  // namespace script_functions
}  // namespace redux

#endif  // REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_MESSAGE_H_
