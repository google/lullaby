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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_STACK_H_
#define REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_STACK_H_

#include <vector>

#include "absl/container/flat_hash_map.h"
#include "redux/engines/script/redux/script_types.h"
#include "redux/modules/base/hash.h"

namespace redux {

// Maps HashValue keys to ScriptValues with support for scoping.
//
// Variables added to the ScriptStack are associated with the "current" scope.
// Scopes can be pushed and popped from the stack. All values added at a given
// scope are removed when the scope is popped. Furthermore, a value with a
// specified key can be added at a given scope such that it does not override
// the value with the same key at a lower scope. This allows two different
// scopes to both declare a variable with the same name.
class ScriptStack {
 public:
  ScriptStack();

  ScriptStack(const ScriptStack& rhs) = delete;
  ScriptStack& operator=(const ScriptStack& rhs) = delete;

  // Sets a value associated with the symbol. If there is no binding for the
  // symbol, a new binding will be introduce in the current scope.
  void SetValue(HashValue id, ScriptValue value);

  // Like SetValue, but introduces a new binding if the symbol doesn't exist
  // in the current scope (even if it exists in a parent scope).
  void LetValue(HashValue id, ScriptValue value);

  // Gets a value associated with the symbol,
  ScriptValue GetValue(HashValue id);

  // Indicates the start of a new scope. Any values set at this scope will not
  // replace values in a prior scope, even if they have the same key.
  void PushScope();

  // Pops the current scope. Any values set at the current scope will be
  // removed.
  void PopScope();

 private:
  // There are two main data structures that are used to store data. The actual
  // Vars are stored in a std::vector which allows for an efficient mechanism
  // for popping the scope. A flat_hash_map is used to as a lookup table for
  // efficiently locating all the Vars (in all scopes) associated with a symbol.

  // Stores the indices of all Vars associated with the same symbol.
  struct IndexArray {
    static const int kMaxInstancesPerValue = 16;

    // Indices into the values_ vector below.
    size_t index[kMaxInstancesPerValue];
    // Number of valid indices in the above |index| array.
    size_t count = 0;
  };

  using LookupTable = absl::flat_hash_map<HashValue, IndexArray>;

  // Stores the actual Var associated with a symbol at a specific scope,
  // as well the symbol's corresponding IndexArray in the lookup table.
  struct ValueEntry {
    ValueEntry(ScriptValue value, LookupTable::value_type* lookup)
        : value(std::move(value)), lookup_entry(lookup) {}

    // The actual Var associated with a symbol.
    ScriptValue value;
    // A pointer to the data in the LookupTable associated with the symbol.
    LookupTable::value_type* lookup_entry;
  };

  // Storage for all the Vars stored in the table for all scopes.
  std::vector<ValueEntry> values_;
  // Lookup table for finding the Vars associated with a given symbol.
  LookupTable lookup_;
  // An index into the values_ table that represents the starting index of a
  // given scope.
  std::vector<size_t> scopes_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_STACK_H_
