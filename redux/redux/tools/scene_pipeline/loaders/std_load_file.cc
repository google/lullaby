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

#include "redux/tools/scene_pipeline/loaders/std_load_file.h"

#include <fstream>
#include <ios>
#include <string>
#include <string_view>

#include "redux/tools/scene_pipeline/buffer.h"
#include "absl/log/check.h"

namespace redux::tool {

Buffer StdLoadFile(std::string_view filename) {
  std::ifstream file(std::string(filename), std::ios::binary);
  CHECK(file) << "File does not exist: " << filename;

  file.seekg(0, std::ios::end);
  const std::streamoff length = file.tellg();
  CHECK_GT(length, 0) << "File is empty: " << filename;

  Buffer buffer(length);
  file.seekg(0, std::ios::beg);
  file.read(reinterpret_cast<char*>(buffer.data()), length);
  file.close();
  return buffer;
}

}  // namespace redux::tool
