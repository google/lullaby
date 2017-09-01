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

// This file implements the following script functions:
//
// (map [(key value)] [(key value)] ... )
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

namespace lull {
namespace {

Optional<HashValue> GetKey(ScriptArgList* args) {
  Optional<HashValue> result;
  ScriptValue key_value = args->EvalNext();
  if (key_value.Is<HashValue>()) {
    result = *key_value.Get<HashValue>();
  } else if (key_value.Is<int>()) {
    result = static_cast<HashValue>(*key_value.Get<int>());
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

void MapSize(ScriptFrame* frame) {
  const ScriptValue map_value = frame->EvalNext();
  const VariantMap* map = map_value.Get<VariantMap>();
  if (map) {
    frame->Return(static_cast<int>(map->size()));
  } else {
    frame->Error("map-size: expected map as first argument");
  }
}

void MapEmpty(ScriptFrame* frame) {
  const ScriptValue map_value = frame->EvalNext();
  const VariantMap* map = map_value.Get<VariantMap>();
  if (map) {
    frame->Return(map->empty());
  } else {
    frame->Error("map-empty: expected map as first argument");
  }
}

void MapInsert(ScriptFrame* frame) {
  ScriptValue map_value = frame->EvalNext();
  VariantMap* map = map_value.Get<VariantMap>();
  if (!map) {
    frame->Error("map-insert: expected map as first argument");
    return;
  }
  Optional<HashValue> key = GetKey(frame);
  if (!key) {
    frame->Error("map-insert: expected key as second argument");
    return;
  }
  const ScriptValue value = frame->EvalNext();
  map->emplace(*key, value.IsNil() ? Variant() : *value.GetVariant());
}

void MapErase(ScriptFrame* frame) {
  ScriptValue map_value = frame->EvalNext();
  VariantMap* map = map_value.Get<VariantMap>();
  if (!map) {
    frame->Error("map-erase: expected map as first argument");
    return;
  }
  Optional<HashValue> key = GetKey(frame);
  if (!key) {
    frame->Error("map-erase: expected key as second argument");
    return;
  }
  auto iter = map->find(*key);
  if (iter != map->end()) {
    map->erase(iter);
  } else {
    frame->Error("map-get: no element at given key");
  }
}

void MapGet(ScriptFrame* frame) {
  const ScriptValue map_value = frame->EvalNext();
  const VariantMap* map = map_value.Get<VariantMap>();
  if (!map) {
    frame->Error("map-get: expected map as first argument");
    return;
  }
  Optional<HashValue> key = GetKey(frame);
  if (!key) {
    frame->Error("map-get: expected key as second argument");
    return;
  }
  auto iter = map->find(*key);
  if (iter != map->end()) {
    frame->Return(iter->second);
  } else {
    frame->Error("map-get: no element at given key");
  }
}

LULLABY_SCRIPT_FUNCTION(MapCreate, "map");
LULLABY_SCRIPT_FUNCTION(MapSize, "map-size");
LULLABY_SCRIPT_FUNCTION(MapEmpty, "map-empty");
LULLABY_SCRIPT_FUNCTION(MapInsert, "map-insert");
LULLABY_SCRIPT_FUNCTION(MapErase, "map-erase");
LULLABY_SCRIPT_FUNCTION(MapGet, "map-get");

}  // namespace
}  // namespace lull
