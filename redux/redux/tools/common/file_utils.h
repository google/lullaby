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

#ifndef REDUX_TOOLS_COMMON_FILE_UTILS_
#define REDUX_TOOLS_COMMON_FILE_UTILS_

#include <functional>
#include <string>

#include "absl/status/statusor.h"
#include "redux/modules/base/data_container.h"

namespace redux::tool {

// Creates the specified directory along with all subdirectories if necessary.
bool CreateFolder(const char* directory);

// Returns true if the specified file exists.
bool FileExists(const char* filename);

// Loads the specified file as a byte array.
absl::StatusOr<DataContainer> LoadFile(const char* filename);

// Loads the specified file into a string.
std::string LoadFileAsString(const char* filename);

// Copies the file from 'src' to 'dst', returning true if successful.
bool CopyFile(const char* dst, const char* src);

// Saves the specified data to the file.
bool SaveFile(const void* bytes, size_t num_bytes, const char* filename,
              bool binary);

// Sets a custom load function. By default, standard C++ loading (eg. fstream)
// is used, and passing null here restores that.
using LoadFileFn =
    std::function<absl::StatusOr<DataContainer>(const char* filename)>;
void SetLoadFileFunction(LoadFileFn fn);

// The default LoadFile function that uses fstream to load files.
absl::StatusOr<DataContainer> DefaultLoadFile(const char* filename);

}  // namespace redux::tool

#endif  // REDUX_TOOLS_COMMON_FILE_UTILS_
