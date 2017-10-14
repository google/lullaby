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

#include "lullaby/modules/script/lull/functions/functions.h"
#include "lullaby/modules/script/lull/script_env.h"
#include "lullaby/modules/script/lull/script_frame.h"
#include "lullaby/modules/script/lull/script_types.h"
#include "lullaby/util/variant.h"

// This file implements the following script functions:
//
// (make-map [(key value)] [(key value)] ... )
//   Creates a map/dictionary with the optional list of key/value pairs.  Each
//   pair must be specified as a tuple (ie. within parentheses).  The keys
//   must be integer or hashvalue types.
//
// (map-size [map])
//   Returns the number of elements in the map.
//
// (map-empty [map])
//   Returns true if the map is empty (ie. contains no elements).
//
// (map-insert [map] [key] [value])
//   Inserts a value into the map at the specified key.  The key must be an
//   integer or hashvalue type.
//
// (map-erase [map] [key])
//   Removes the element in the map specified by the key.  The key must be an
//   integer or hashvalue type.
//
// (map-get [map] [key])
//   Returns the value associated with the key in the map.  The key must be an
//   integer or hashvalue type.
//
// (map-set [map] [key] [value])
//    Sets the value associated with the key in the map.  The key must be an
//    integer or hashvalue type.
//
// (map-foreach [map] ([key-name] [value-name]) [expressions...])
//   Passes each element of the map to expressions with the key bound to
//   [key-name] and the value bound to [value-name].

namespace lull {
namespace {

Optional<HashValue> GetKey(ScriptArgList* args) {
  Optional<HashValue> result;
  ScriptValue key_value = args->EvalNext();
  if (key_value.Is<HashValue>()) {
    result = *key_value.Get<HashValue>();
  }
  return result;
}

Variant GetValue(ScriptArgList* args) {
  const ScriptValue value = args->EvalNext();
  return value.IsNil() ? Variant() : *value.GetVariant();
}

void MapCreate(ScriptFrame* frame) {
  VariantMap map;
  while (frame->HasNext()) {
    ScriptValue arg = frame->Next();
    const AstNode* node = arg.Get<AstNode>();
    if (!node) {
      frame->Error("map: expected tuple as map arguments");
      break;
    }

    ScriptArgList tuple(frame->GetEnv(), node->first);
    Optional<HashValue> key = GetKey(&tuple);
    if (!key) {
      break;
    }
    map.emplace(*key, GetValue(&tuple));
  }
  frame->Return(map);
}

int MapSize(const VariantMap* map) { return static_cast<int>(map->size()); }
bool MapEmpty(const VariantMap* map) { return map->empty(); }

void MapInsert(VariantMap* map, HashValue key, const Variant* value) {
  map->emplace(key, *value);
}

void MapErase(ScriptFrame* frame, VariantMap* map, HashValue key) {
  auto iter = map->find(key);
  if (iter != map->end()) {
    map->erase(iter);
  } else {
    frame->Error("map-erase: no element at given key");
  }
}

Variant MapGetOr(const VariantMap* map, HashValue key,
                 const Variant* default_value) {
  auto iter = map->find(key);
  if (iter != map->end()) {
    return iter->second;
  }
  return *default_value;
}

Variant MapGet(ScriptFrame* frame, const VariantMap* map, HashValue key) {
  Variant v;
  v = MapGetOr(map, key, &v);
  if (v.Empty()) {
    frame->Error("map-get: no element at given key");
  }
  return v;
}

void MapSet(VariantMap* map, HashValue key, const Variant* value) {
  (*map)[key] = *value;
}

void MapForeach(ScriptFrame* frame) {
  if (!frame->HasNext()) {
    frame->Error("map-foreach: expect [map] ([args]) [body].");
    return;
  }
  ScriptValue map_arg = frame->EvalNext();
  if (!map_arg.Is<VariantMap>()) {
    frame->Error("map-foreach: first argument should be a map.");
    return;
  }
  const VariantMap* map = map_arg.Get<VariantMap>();

  const AstNode* node = frame->GetArgs().Get<AstNode>();
  if (!node) {
    frame->Error("map-foreach: expected parameters after map.");
    return;
  }

  ScriptEnv* env = frame->GetEnv();
  const AstNode* params = node->first.Get<AstNode>();
  const Symbol* key = params ? params->first.Get<Symbol>() : nullptr;
  params = params ? params->rest.Get<AstNode>() : nullptr;
  const Symbol* value = params ? params->first.Get<Symbol>() : nullptr;

  if (!key || !value) {
    frame->Error("map-foreach: should be at least 2 symbol parameters");
    return;
  }

  // Now iterate the map elements, evaluating body.
  ScriptValue result;
  for (const auto& kv : *map) {
    env->SetValue(*key, env->Create(kv.first));
    env->SetValue(*value, env->Create(kv.second));
    ScriptValue iter = node->rest;
    while (auto* node = iter.Get<AstNode>()) {
      result = env->Eval(iter);
      // Maybe implement continue/break here?
      iter = node->rest;
    }
  }
  frame->Return(result);
}

LULLABY_SCRIPT_FUNCTION(MapCreate, "make-map");
LULLABY_SCRIPT_FUNCTION_WRAP(MapSize, "map-size");
LULLABY_SCRIPT_FUNCTION_WRAP(MapEmpty, "map-empty");
LULLABY_SCRIPT_FUNCTION_WRAP(MapInsert, "map-insert");
LULLABY_SCRIPT_FUNCTION_WRAP(MapErase, "map-erase");
LULLABY_SCRIPT_FUNCTION_WRAP(MapGet, "map-get");
LULLABY_SCRIPT_FUNCTION_WRAP(MapGetOr, "map-get-or");
LULLABY_SCRIPT_FUNCTION_WRAP(MapSet, "map-set");
LULLABY_SCRIPT_FUNCTION(MapForeach, "map-foreach");

}  // namespace
}  // namespace lull
