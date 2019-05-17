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

#ifndef LULLABY_UTIL_FILENAME_H_
#define LULLABY_UTIL_FILENAME_H_

#include <string>
#include "lullaby/util/string_view.h"

namespace lull {

// Checks if the |filename| has the filetype |suffix|, which should include the
// '.' (e.g. ".wav", not "wav").
bool EndsWith(string_view filename, string_view suffix);

// Gets the filename and extension from a file path. In other words, strips
// the directory from the file path. For example:
//   "lullaby/foo/bar.png" returns "bar.png".
//   "lullaby/foo/bar" returns "bar".
//   "lullaby/foo/" returns ""
std::string GetBasenameFromFilename(string_view filename);

// Gets the extension (including the dot) from a file path. For example:
//   "lullaby/foo/bar.png" returns ".png".
//   "lullaby/foo/" returns ""
std::string GetExtensionFromFilename(string_view filename);

// Removes the extension a file path. For example:
//   "lullaby/foo/bar.png" returns "lullaby/foo/bar".
//   "lullaby/foo/" returns "lullaby/foo/"
std::string RemoveExtensionFromFilename(string_view filename);

// Removes both the directory and the extension a file path. For example:
//   "lullaby/foo/bar.png" returns "bar".
//   "lullaby/foo/" returns ""
std::string RemoveDirectoryAndExtensionFromFilename(string_view filename);

// Returns the entire file path up to the last directory (without the trailing
// directory separator).  For example:
//   "lullaby/foo/bar.png" returns "lullaby/foo".
//   "lullaby/foo/" returns "lullaby/foo"
std::string GetDirectoryFromFilename(string_view filename);

// Joins a Directory and Basename into a Filename.  For example:
//   ("lullaby/foo", "bar.png") returns "lullaby/foo/bar.png".
std::string JoinPath(string_view directory, string_view basename);

// Correct for platform differences in expressing a path (e.g. a file exported
// on windows may try to locate textures\\file.png on linux/mac).
std::string LocalizePath(string_view path);

// Canonicalize the path.  Basically convert directory\file to
// directory/file.
std::string CanonicalizePath(string_view path);

}  // namespace lull

#endif  // LULLABY_UTIL_FILENAME_H_
