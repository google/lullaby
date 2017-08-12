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

#ifndef LULLABY_MODULES_SCRIPT_LULL_SCRIPT_SCOPED_SYMBOL_TABLE_H_
#define LULLABY_MODULES_SCRIPT_LULL_SCRIPT_SCOPED_SYMBOL_TABLE_H_

#include <unordered_map>
#include <vector>
#include "lullaby/modules/script/lull/script_value.h"
#include "lullaby/util/hash.h"

namespace lull {

// Maps HashValue keys to ScriptValue instances.
//
// In addition to being a map, the ScriptScopedSymbolTable also understands
// scope.  ScriptScopedSymbolTable effectively act like the "stack" for the
// ScriptEnv.  Variables added to the ScriptScopedSymbolTable are associated
// with the "current" scope.  Scopes can be pushed and popped from the table.
// All values added at a given scope are removed when the scope is popped.
// Furthermore, a value with a specified key that is added at a given scope does
// not override the value with the same key at a lower scope.  This allows two
// different scopes to both declare a variable with the same name.
class ScriptScopedSymbolTable {
 public:
  ScriptScopedSymbolTable();

  // Sets a value associated with the symbol.
  void SetValue(HashValue symbol, ScriptValue value);

  // Gets a value associated with the symbol,
  ScriptValue GetValue(HashValue symbol) const;

  // Indicates the start of a new scope.  Any values set at this scope will not
  // replace values in a prior scope, even if they have the same key.
  void PushScope();

  // Pops the current scope.  Any values set at the current scope will be
  // removed.
  void PopScope();

 private:
  // There are two main data structures that are used to represent the data
  // stored in the ScriptScopedSymbolTable.  The actual ScriptValues are stored
  // in a std::vector.  This allows for an efficient mechanism for removing all
  // ScriptValues when a scope is popped.  A std::unordered_map is used to
  // as a lookup table for efficiently locating all the ScriptValues (in all
  // scopes) associated with a symbol.

  // Stores the indices of all ScriptValues associated with the same symbol.
  struct IndexArray {
    static const int kMaxInstancesPerValue = 16;

    // Indices into the values_ vector below.
    size_t index[kMaxInstancesPerValue];
    // Number of valid indices in the above |index| array.
    size_t count = 0;
  };

  using LookupTable = std::unordered_map<HashValue, IndexArray>;

  // Stores the actual ScriptValue associated with a symbol at a specific scope,
  // as well the symbol's corresponding IndexArray in the lookup table.
  struct ValueEntry {
    ValueEntry(ScriptValue value, LookupTable::value_type* lookup)
        : value(std::move(value)), lookup_entry(lookup) {}

    // The actual ScriptValue associated with a symbol.
    ScriptValue value;
    // A pointer to the data in the LookupTable associated with the symbol.
    LookupTable::value_type* lookup_entry;
  };

  // Storage for all the ScriptValues stored in the table for all scopes.
  std::vector<ValueEntry> values_;
  // Lookup table for finding the ScriptValues associated with a given symbol.
  LookupTable lookup_;
  // An index into the values_ table that represents the starting index of a
  // given scope.
  std::vector<size_t> scopes_;
};

}  // namespace lull

#endif  // LULLABY_MODULES_SCRIPT_LULL_SCRIPT_SCOPED_SYMBOL_TABLE_H_
