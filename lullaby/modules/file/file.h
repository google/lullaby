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

#ifndef LULLABY_MODULES_FILE_FILE_H_
#define LULLABY_MODULES_FILE_FILE_H_

#include <string>

namespace lull {

// Check if the file |filename| has the filetype |suffix|, which should include
// a '.' (e.g. ".wav", not "wav")
bool EndsWith(const std::string& filename, const std::string& suffix);

// Gets the filename and extension from a file path. For example:
//   "lullaby/foo/bar.png" returns "bar.png".
//   "lullaby/foo/" returns ""
std::string GetBasenameFromFilename(const std::string& filename);

// Gets the extension (incl. the dot) from a file path. For example:
//   "lullaby/foo/bar.png" returns ".png".
//   "lullaby/foo/" returns ""
std::string GetExtensionFromFilename(const std::string& filename);

// Removes the extension a file path. For example:
//   "lullaby/foo/bar.png" returns "lullaby/foo/bar".
//   "lullaby/foo/" returns "lullaby/foo/"
std::string RemoveExtensionFromFilename(const std::string& filename);

// Returns the entire file path up to the last directory.  For example:
//   "lullaby/foo/bar.png" returns "lullaby/foo".
//   "lullaby/foo/" returns "lullaby/foo"
std::string GetDirectoryFromFilename(const std::string& filename);

}  // namespace lull

#endif  // LULLABY_MODULES_FILE_FILE_H_
