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

#ifndef REDUX_MODULES_VAR_VAR_SERIALIZER_H_
#define REDUX_MODULES_VAR_VAR_SERIALIZER_H_

#include <vector>

#include "redux/modules/var/var.h"
#include "redux/modules/var/var_convert.h"
#include "redux/modules/var/var_table.h"

namespace redux {

template <bool kIsDestructive>
class VarSerializer {
 public:
  explicit VarSerializer(Var* var) : root_(var) {
    CHECK(root_) << "Must provide valid root.";
  }

  void Begin(HashValue key) {
    if (stack_.empty()) {
      stack_.push_back(root_);
    } else {
      Var& back = *stack_.back();
      stack_.push_back(&back[key]);
    }
    if constexpr (!kIsDestructive) {
      *stack_.back() = VarTable();
    }
  }

  void End() { stack_.pop_back(); }

  template <typename T>
  void operator()(T& value, HashValue key) {
    Var& back = *stack_.back();
    if constexpr (kIsDestructive) {
      FromVar(back[key], &value);
    } else {
      ToVar(value, &back[key]);
    }
  }

  constexpr bool IsDestructive() { return kIsDestructive; }

 protected:
  Var* root_;
  std::vector<Var*> stack_;
};

using SaveToVar = VarSerializer<false>;
using LoadFromVar = VarSerializer<true>;

}  // namespace redux

#endif  // REDUX_MODULES_VAR_VAR_SERIALIZER_H_
