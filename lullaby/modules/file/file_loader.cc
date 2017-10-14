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

#include "lullaby/modules/file/file_loader.h"

#include <fstream>

#include "fplbase/utilities.h"
#include "lullaby/util/logging.h"

namespace lull {

bool LoadAssetOrFile(const char* filename, std::string* dest) {
  // Attempts to load using fplbase::LoadFileRaw if the filename is local.
  if (filename[0] != '/') {
    return fplbase::LoadFileRaw(filename, dest);
  }
  // Load from the file system if we start with '/' (we shouldn't be depending
  // on current directory).
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    LOG(ERROR) << "Failed to open file " << filename;
    return false;
  }

  file.seekg(0, std::ios::end);
  std::streamoff length = file.tellg();
  file.seekg(0, std::ios::beg);
  if (length < 0) {
    LOG(ERROR) << "Failed to get file size for " << filename;
    return false;
  }

  dest->resize(static_cast<size_t>(length));
  file.read(&(*dest)[0], dest->size());
  return file.good();
}

}  // namespace lull
