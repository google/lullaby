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

#include "lullaby/modules/script/lull/script_env.h"
#include "lullaby/modules/script/lull/script_frame.h"
#include "lullaby/modules/script/lull/script_types.h"

namespace lull {

static Optional<HashValue> GetKeyFromScriptValue(const ScriptValue& key_value) {
  Optional<HashValue> result;
  if (key_value.Is<HashValue>()) {
    result = *key_value.Get<HashValue>();
  } else if (key_value.Is<int>()) {
    result = static_cast<HashValue>(*key_value.Get<int>());
  }
  return result;
}

void CreateMap(ScriptFrame* frame) {
  VariantMap map;
  while (frame->HasNext()) {
    // Each argument to a map is itself an argument list, specifically a
    // key/value tuple.
    ScriptArgList tuple(frame->GetEnv(), frame->Next());

    ScriptValue key_value;
    if (tuple.HasNext()) {
      key_value = tuple.Next();
    } else {
      frame->Error("Expected key as first element in tuple.");
      break;
    }

    auto key = GetKeyFromScriptValue(key_value);
    if (!key) {
      frame->Error("Key must be of type int or HashValue.");
      break;
    }

    ScriptValue value;
    if (tuple.HasNext()) {
      value = tuple.EvalNext();
    } else {
      frame->Error("Expected value as second element in tuple.");
      break;
    }

    if (tuple.HasNext()) {
      frame->Error("Too many elements in map tuple");
      break;
    }
    map[*key] = *value.GetVariant();
  }
  frame->Return(map);
}

}  // namespace lull
