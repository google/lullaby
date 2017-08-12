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

#ifndef LULLABY_MODULES_DEBUG_LOG_TAG_H_
#define LULLABY_MODULES_DEBUG_LOG_TAG_H_

#include "lullaby/util/hash.h"
#include "lullaby/util/string_view.h"

// TagTree class provides a tree data structure to enable better logging
// functionality. All logs will be required to provide a string tag specifying
// the feature to which they belong.

namespace lull {
namespace debug {

// Tag is public for testing.
struct Tag {
  Tag() : value(0) {}
  explicit Tag(const string_view tag_name)
      : name(tag_name),
        value(HashCaseInsensitive(tag_name.data(), tag_name.length())) {}
  string_view name;
  HashValue value;
};

// Exposed for testing purposes. Splits string_view on "." and creates
// an array of separate tags.
size_t SplitTag(string_view tag_name, Tag* out, size_t num);

// Initializes tag structure.
void InitializeLogTag();

// Resets tag strcture to nullptr.
void ShutdownLogTag();

// Enables tag if it exists. Splits tag on "." and creates a new tree branch
// out of each sub tag if it doesn't. "lull.Transform.SetSqt" will become
// separate nodes with "lull"->"Transform" -> "SetSqt". Preserves children tags
// boolean statuses.
void Enable(string_view tag);

// Enables tag and all of its children if it exists.
void EnableBranch(string_view tag);

// Disables tag if it exists. Creates a new tree branch if it doesn't,
// where a specified tag is disabled, but its children are unaffected. Preserves
// children tags boolean statuses.
void Disable(string_view tag);

// Disables tag and all of its children if it exists.
void DisableBranch(string_view tag);

// Returns bool of full tag enabled status if it exists. Creates a new tree
// branch if it doesn't, where a specified tag is disabled. Does not affect
// parent tags if any.
bool IsEnabled(string_view tag);

}  // namespace debug
}  // namespace lull

#endif  // LULLABY_MODULES_DEBUG_LOG_TAG_H_
