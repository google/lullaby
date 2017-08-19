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

#ifndef LULLABY_UTIL_SELECTOR_H_
#define LULLABY_UTIL_SELECTOR_H_

#include "lullaby/util/optional.h"
#include "lullaby/util/span.h"
#include "lullaby/util/variant.h"

namespace lull {

// Interface for selecting an element from a set based on arbitrary criteria.
template <typename T>
struct Selector {
  using Type = T;

  virtual ~Selector() {}

  // Returns the index of the element in |choices| that "matches" the criteria
  // specified by |args|.
  virtual Optional<size_t> Select(const VariantMap& params,
                                  Span<Type> choices) = 0;
};

}  // namespace lull

#endif  // LULLABY_UTIL_SELECTOR_H_
