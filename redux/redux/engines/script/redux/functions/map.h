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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_MAP_H_
#define REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_MAP_H_

#include "redux/engines/script/redux/script_env.h"

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
//
// Note that redux script parsing uses { and } as a short-cut for make-map.
// In other words, the following are equivalent:
//   (make-map (:city 'new york') (:country 'usa'))
//   {(:city 'new york') (:country 'usa')}

#include "redux/modules/var/var_table.h"

namespace redux {
namespace script_functions {

inline void MapCreateFn(ScriptFrame* frame) {
  VarTable map;
  while (frame->HasNext()) {
    const ScriptValue& arg = frame->Next();
    const AstNode* node = arg.Get<AstNode>();
    if (!node) {
      frame->Error("map: expected tuple as map arguments");
      break;
    }

    ScriptFrame tuple(frame->GetEnv(), node->first);
    ScriptValue key = tuple.EvalNext();
    ScriptValue value = tuple.EvalNext();
    if (key) {
      uint32_t id = 0;
      key.GetAs(&id);
      map.Insert(HashValue(id), *value.Get<Var>());
    } else {
      CHECK(false);
    }
  }
  frame->Return(map);
}

inline int MapSizeFn(const VarTable* map) { return map->Count(); }

inline bool MapEmptyFn(const VarTable* map) { return map->Count() == 0; }

inline void MapInsertFn(VarTable* map, HashValue key, const Var* value) {
  map->Insert(key, *value);
}

inline void MapEraseFn(VarTable* map, HashValue key) { map->Erase(key); }

inline Var MapGetOrFn(const VarTable* map, HashValue key, const Var* default_value) {
  const Var* var = map->TryFind(key);
  if (var) {
    return *var;
  }
  return *default_value;
}

inline Var MapGetFn(const VarTable* map, HashValue key) {
  Var v;
  return MapGetOrFn(map, key, &v);
}

inline void MapSetFn(VarTable* map, HashValue key, const Var* value) {
  (*map)[key] = *value;
}

inline void MapForEachFn(ScriptFrame* frame) {
  if (!frame->HasNext()) {
    frame->Error("map-foreach: expect [map] ([args]) [body].");
    return;
  }
  ScriptValue map_arg = frame->EvalNext();
  if (!map_arg.Is<VarTable>()) {
    frame->Error("map-foreach: first argument should be a map.");
    return;
  }
  const VarTable* map = map_arg.Get<VarTable>();

  const AstNode* node = frame->GetArgs().Get<AstNode>();
  if (!node) {
    frame->Error("map-foreach: expected parameters after map.");
    return;
  }

  const AstNode* params = node->first.Get<AstNode>();
  const Symbol* key = params ? params->first.Get<Symbol>() : nullptr;
  params = params && params->rest ? params->rest.Get<AstNode>() : nullptr;
  const Symbol* value = params ? params->first.Get<Symbol>() : nullptr;

  if (!key || !value) {
    frame->Error("map-foreach: should be at least 2 symbol parameters");
    return;
  }

  ScriptEnv* env = frame->GetEnv();

  // Now iterate the map elements, evaluating body.
  ScriptValue result;
  for (const auto& kv : *map) {
    env->SetValue(key->value, kv.first);
    env->SetValue(value->value, kv.second);

    const ScriptValue* iter = &node->rest;
    while (auto* node = iter->Get<AstNode>()) {
      result = env->Eval(*iter);
      // Maybe implement continue/break here?
      iter = &node->rest;
      if (iter->IsNil()) {
        break;
      }
    }
  }
  frame->Return(result);
}
}  // namespace script_functions
}  // namespace redux

#endif  // REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_MAP_H_
