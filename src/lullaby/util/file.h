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

#ifndef LULLABY_UTIL_FILE_H_
#define LULLABY_UTIL_FILE_H_

#include <string>

namespace lull {

// Check if the file |filename| has the filetype |suffix|, which should include
// a '.' (e.g. ".wav", not "wav")
bool EndsWith(const std::string& filename, const std::string& suffix);

// Gets the filename and extension from a file path.
// i.e. "lullaby/textures/foo.png" returns "foo.png".
// returns "" if the path ends with a "/"
const std::string GetBasenameFromFilename(const std::string& filename);
const std::string GetExtensionFromFilename(const std::string& filename);

}  // namespace lull

#endif  // LULLABY_UTIL_FILE_H_
