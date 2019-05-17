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

#include "lullaby/util/filename.h"

namespace lull {

namespace {

#if defined(_WINDOWS) || defined(_WIN32)
const char kPathDelimiter = '\\';
const char kPathDelimiterNonlocal = '/';
#else
const char kPathDelimiter = '/';
const char kPathDelimiterNonlocal = '\\';
#endif  // defined(_WINDOWS) || defined(_WIN32)

size_t find_last_of(string_view str, string_view chars) {
  for (int index = static_cast<int>(str.size()) - 1; index >= 0; --index) {
    for (char c : chars) {
      if (str[index] == c) {
        return index;
      }
    }
  }
  return string_view::npos;
}

}  // namespace

bool EndsWith(string_view filename, string_view suffix) {
  const size_t size = filename.size();
  const size_t suffix_size = suffix.size();
  if (size < suffix_size) {
    return false;
  }
  const string_view tail = filename.substr(size - suffix_size, suffix_size);
  return tail.compare(suffix) == 0;
}

std::string GetBasenameFromFilename(string_view filename) {
  const size_t index = find_last_of(filename, "/\\");
  if (index == std::string::npos) {
    return std::string(filename);
  }
  if (index - 1 == filename.length()) {
    return "";
  }
  return std::string(filename.substr(index + 1));
}

std::string GetExtensionFromFilename(string_view filename) {
  const size_t index = find_last_of(filename, ".");
  if (index == std::string::npos) {
    return "";
  }
  return std::string(filename.substr(index));
}

std::string RemoveExtensionFromFilename(string_view filename) {
  const size_t index = find_last_of(filename, ".");
  if (index == std::string::npos) {
    return std::string(filename);
  }
  return std::string(filename.substr(0, index));
}

std::string RemoveDirectoryAndExtensionFromFilename(string_view filename) {
  return GetBasenameFromFilename(RemoveExtensionFromFilename(filename));
}

std::string GetDirectoryFromFilename(string_view filename) {
  const size_t index = find_last_of(filename, "/\\");
  if (index == std::string::npos) {
    return "";
  }
  return std::string(filename.substr(0, index));
}

std::string JoinPath(string_view directory, string_view basename) {
  std::string prefix(directory);
  std::string suffix(basename);

  // Ensure directory does not have a trailing slash.
  std::string cleaned_directory =
      (!prefix.empty() &&
       prefix.find_first_of("/\\", prefix.length() - 1) != std::string::npos)
          ? prefix.substr(0, prefix.length() - 1)
          : prefix;

  // Ensure basename does not have a leading slash (unless dirname is empty,
  // in which case we treat basename as a full path).
  std::string cleaned_basename =
      (!suffix.empty() && !directory.empty() &&
       suffix.find_last_of("/\\", 0) != std::string::npos)
          ? suffix.substr(1, std::string::npos)
          : suffix;

  // Combine the cleaned directory and base names.  For consistency, we emit
  // local paths (e.g. './foo.txt') without the leading './'.
  return (cleaned_directory == "." || cleaned_directory.empty())
             ? cleaned_basename
             : cleaned_directory + kPathDelimiter + cleaned_basename;
}

std::string LocalizePath(string_view path) {
  std::string str_path(path);
  auto cursor = str_path.find_first_of(kPathDelimiterNonlocal, 0);
  while (cursor != std::string::npos) {
    str_path.replace(cursor, 1, 1, kPathDelimiter);
    cursor = str_path.find_first_of(kPathDelimiterNonlocal, cursor);
  }
  return str_path;
}

std::string CanonicalizePath(string_view path) {
  std::string str_path(path);
  auto cursor = str_path.find_first_of('\\', 0);
  while (cursor != std::string::npos) {
    str_path.replace(cursor, 1, 1, '/');
    cursor = str_path.find_first_of('\\', cursor);
  }
  return str_path;
}

}  // namespace lull
