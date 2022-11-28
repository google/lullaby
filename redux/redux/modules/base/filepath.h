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

#ifndef REDUX_MODULES_BASE_FILEPATH_H_
#define REDUX_MODULES_BASE_FILEPATH_H_

#include <string>
#include <string_view>

namespace redux {

// Checks if the `filepath` has the type `suffix`, which should include the '.'
// (e.g. ".wav", not "wav").
bool EndsWith(std::string_view filename, std::string_view suffix);

// Gets the filepath and extension from a file path. In other words, strips
// the directory from the file path. For example:
//   "redux/foo/bar.png" returns "bar.png".
//   "redux/foo/bar" returns "bar".
//   "redux/foo/" returns ""
std::string_view GetBasepath(std::string_view filename);

// Gets the extension (including the dot) from a file path. For example:
//   "redux/foo/bar.png" returns ".png".
//   "redux/foo/" returns ""
std::string_view GetExtension(std::string_view filename);

// Returns the entire file path up to the last directory (without the trailing
// directory separator).  For example:
//   "redux/foo/bar.png" returns "redux/foo".
//   "redux/foo/" returns "redux/foo"
std::string_view GetDirectory(std::string_view filename);

// Removes the extension a file path. For example:
//   "redux/foo/bar.png" returns "redux/foo/bar".
//   "redux/foo/" returns "redux/foo/"
std::string_view RemoveExtension(std::string_view filename);

// Removes both the directory and the extension a file path. For example:
//   "redux/foo/bar.png" returns "bar".
//   "redux/foo/" returns ""
std::string_view RemoveDirectoryAndExtension(std::string_view filename);

// Joins a Directory and Basename into a filepath.  For example:
//   ("redux/foo", "bar.png") returns "redux/foo/bar.png".
std::string JoinPath(std::string_view directory, std::string_view basename);

// Correct for platform differences in expressing a path (e.g. a file exported
// on windows may try to locate textures\\file.png on linux/mac).
std::string LocalizePath(std::string_view path);

// Canonicalize the path regardless of platform. Basically convert
// 'directory\file' to 'directory/file'.
std::string CanonicalizePath(std::string_view path);

}  // namespace redux

#endif  // REDUX_MODULES_BASE_FILEPATH_H_
