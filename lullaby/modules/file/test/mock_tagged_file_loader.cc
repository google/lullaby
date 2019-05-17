/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#include "lullaby/modules/file/test/mock_tagged_file_loader.h"

namespace lull {

bool MockTaggedFileLoader::PlatformSpecificLoadFile(
    const char* filename, std::string* dest, const std::string& tag_used) {
  return load_fn_(filename, dest, tag_used);
}

void MockTaggedFileLoader::RegisterTag(const std::string& tag,
                                       const std::string& path_prefix) {
  TaggedFileLoader::RegisterTag(tag, path_prefix);
}

MockTaggedFileLoader::MockLoadFileFn MockTaggedFileLoader::SetMockLoadFn(
    MockLoadFileFn fn) {
  std::swap(load_fn_, fn);
  return fn;
}

}  // namespace lull
