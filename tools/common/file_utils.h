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

#ifndef LULLABY_TOOLS_COMMON_FILE_UTILS_H_
#define LULLABY_TOOLS_COMMON_FILE_UTILS_H_

#include <string>

namespace lull {
namespace tool {

// Creates the specified directory along with all subdirectories if necessary.
bool CreateFolder(const char* directory);

// Returns true if the specified file exists.
bool FileExists(const char* filename);

// Loads the specified file into |out|. This API matches the function signature
// required by flatbuffers.
bool LoadFile(const char* filename, bool binary, std::string* out);

// Saves the specified data to the file.
bool SaveFile(const void* bytes, size_t num_bytes, const char* filename,
              bool binary);

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_COMMON_FILE_UTILS_H_
