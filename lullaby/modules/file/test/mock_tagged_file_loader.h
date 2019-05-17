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

#ifndef LULLABY_MODULES_FILE_TEST_MOCK_TAGGED_FILE_LOADER_H_
#define LULLABY_MODULES_FILE_TEST_MOCK_TAGGED_FILE_LOADER_H_

#include <functional>
#include <string>

#include "lullaby/modules/file/tagged_file_loader.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Mock TaggedFileLoader implementation for test purposes.
class MockTaggedFileLoader : public TaggedFileLoader {
 public:
  // Mocks loading the contents of the specified filename into the given string
  // pointer. Includes the tag used for test purposes.
  // Return true on success, false otherwise.
  using MockLoadFileFn =
      std::function<bool(const char*, std::string*, const std::string&)>;

  // Calls TaggedFileLoader::RegisterTag(), which is protected since impls may
  // need additional information (such as Android needing an AAssetManager).
  void RegisterTag(const std::string& tag, const std::string& path_prefix);

  // Sets the load function that |LoadFile| will use for tagged paths.
  // Returns the previously set TestLoadFileFn.
  MockLoadFileFn SetMockLoadFn(MockLoadFileFn fn);

 protected:
  // Simply calls the supplied TestLoadFileFn.
  bool PlatformSpecificLoadFile(const char* filename, std::string* dest,
                                const std::string& tag_used) override;

  MockLoadFileFn load_fn_ = nullptr;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::MockTaggedFileLoader);

#endif  // LULLABY_MODULES_FILE_TEST_MOCK_TAGGED_FILE_LOADER_H_
