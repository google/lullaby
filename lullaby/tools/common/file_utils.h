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

#ifndef LULLABY_TOOLS_COMMON_FILE_UTILS_H_
#define LULLABY_TOOLS_COMMON_FILE_UTILS_H_

#include <string>

// TODO: Rename CopyFile and remove this to make windows builds more
// robust.
#ifdef WIN32
#ifdef CopyFile
#undef CopyFile
#endif  // CopyFile
#endif  // WIN32

namespace lull {
namespace tool {

typedef bool (*LoadFileFunction)(const char* filename, bool binary,
                                 std::string* out);

// Sets a custom load function.  By default, plain file system loading is used,
// and passing null here restores that.
void SetLoadFileFunction(LoadFileFunction fn);

// Creates the specified directory along with all subdirectories if necessary.
bool CreateFolder(const char* directory);

// Returns true if the specified file exists.
bool FileExists(const char* filename);

// Loads the specified file into |out|. This API matches the function signature
// required by flatbuffers.
bool LoadFile(const char* filename, bool binary, std::string* out);

// Copies the file from 'src' to 'dst'. Returns true if it is successful.
bool CopyFile(const char* dst, const char* src);

// Saves the specified data to the file.
bool SaveFile(const void* bytes, size_t num_bytes, const char* filename,
              bool binary);

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_COMMON_FILE_UTILS_H_
