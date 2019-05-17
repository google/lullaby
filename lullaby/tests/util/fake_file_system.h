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

#ifndef LULLABY_TESTS_UTIL_FAKE_FILE_SYSTEM_H_
#define LULLABY_TESTS_UTIL_FAKE_FILE_SYSTEM_H_

#include <string>
#include <unordered_map>
#include <vector>

namespace lull {

// A fake file system for use by the AssetLoader to allow creating entities
// from blueprints by name in tests. Note that entity_factory assumes that
// compiled flatbuffers have been saved with a ".bin" suffix.
class FakeFileSystem {
  using DataBuffer = std::vector<uint8_t>;

 public:
  // Add |len| bytes of |data| to the FakeFileSystem under |name|. It can be
  // retrieved with LoadFromDisk().
  void SaveToDisk(const std::string& name, const uint8_t* data, size_t len) {
    file_system_[name] = DataBuffer(data, data + len);
  }
  // Get the data stored previously in the FakeFileSystem under |name| from
  // SaveToDisk() and write it to |out|. Returns true if successful.
  bool LoadFromDisk(const std::string& name, std::string* out) {
    auto iter = file_system_.find(name);
    if (iter == file_system_.end()) {
      return false;
    }

    const DataBuffer& data = iter->second;
    out->resize(data.size());
    memcpy(const_cast<char*>(out->data()), data.data(), data.size());
    return true;
  }

 private:
  std::unordered_map<std::string, DataBuffer> file_system_;
};

}  // namespace lull

#endif  // LULLABY_TESTS_UTIL_FAKE_FILE_SYSTEM_H_
