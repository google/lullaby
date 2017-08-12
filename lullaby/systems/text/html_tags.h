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

#ifndef LULLABY_SYSTEMS_TEXT_HTML_TAGS_H_
#define LULLABY_SYSTEMS_TEXT_HTML_TAGS_H_

#include <string>
#include <vector>

#include "lullaby/util/math.h"

namespace lull {

// Represents a link that was stripped out of a font buffer.
struct LinkTag {
  // The destination of the link.
  std::string href;

  // The location of this tag relative to the entity. Tags that span multiple
  // lines can have multiple bounding boxes.
  std::vector<Aabb> aabbs;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_TEXT_HTML_TAGS_H_
