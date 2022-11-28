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

#include "redux/modules/base/filepath.h"

namespace redux {

#if defined(_WINDOWS) || defined(_WIN32)
static constexpr char kPathDelimiter = '\\';
static constexpr char kPathDelimiterNonlocal = '/';
#else
static constexpr char kPathDelimiter = '/';
static constexpr char kPathDelimiterNonlocal = '\\';
#endif  // defined(_WINDOWS) || defined(_WIN32)

static size_t FindLastOf(std::string_view str, std::string_view chars) {
  for (int index = static_cast<int>(str.size()) - 1; index >= 0; --index) {
    for (char c : chars) {
      if (str[index] == c) {
        return index;
      }
    }
  }
  return std::string_view::npos;
}

bool EndsWith(std::string_view filename, std::string_view suffix) {
  const size_t size = filename.size();
  const size_t suffix_size = suffix.size();
  if (size < suffix_size) {
    return false;
  }
  const std::string_view tail =
      filename.substr(size - suffix_size, suffix_size);
  return tail.compare(suffix) == 0;
}

std::string_view GetBasepath(std::string_view filename) {
  const size_t index = FindLastOf(filename, "/\\");
  if (index == std::string_view::npos) {
    return std::string_view(filename);
  }
  if (index - 1 == filename.length()) {
    return "";
  }
  return std::string_view(filename.substr(index + 1));
}

std::string_view GetExtension(std::string_view filename) {
  const size_t index = FindLastOf(filename, ".");
  if (index == std::string_view::npos) {
    return "";
  }
  return std::string_view(filename.substr(index));
}

std::string_view RemoveExtension(std::string_view filename) {
  const size_t index = FindLastOf(filename, ".");
  if (index == std::string_view::npos) {
    return std::string_view(filename);
  }
  return std::string_view(filename.substr(0, index));
}

std::string_view RemoveDirectoryAndExtension(std::string_view filename) {
  return GetBasepath(RemoveExtension(filename));
}

std::string_view GetDirectory(std::string_view filename) {
  const size_t index = FindLastOf(filename, "/\\");
  if (index == std::string_view::npos) {
    return "";
  }
  return std::string_view(filename.substr(0, index));
}

std::string JoinPath(std::string_view directory, std::string_view basename) {
  std::string_view prefix(directory);
  std::string_view suffix(basename);

  // Ensure directory does not have a trailing slash.
  std::string_view cleaned_directory =
      (!prefix.empty() && prefix.find_first_of("/\\", prefix.length() - 1) !=
                              std::string_view::npos)
          ? prefix.substr(0, prefix.length() - 1)
          : prefix;

  // Ensure basename does not have a leading slash (unless dirname is empty,
  // in which case we treat basename as a full path).
  std::string_view cleaned_basename =
      (!suffix.empty() && !directory.empty() &&
       suffix.find_last_of("/\\", 0) != std::string_view::npos)
          ? suffix.substr(1, std::string_view::npos)
          : suffix;

  // Combine the cleaned directory and base names.  For consistency, we emit
  // local paths (e.g. './foo.txt') without the leading './'.
  if (cleaned_directory == "." || cleaned_directory.empty()) {
    return std::string(cleaned_basename);
  } else {
    const char delim[] = {kPathDelimiter, 0};
    return std::string(cleaned_directory)
        .append(delim)
        .append(std::string(cleaned_basename));
  }
}

std::string LocalizePath(std::string_view path) {
  std::string str_path(path);
  auto cursor = str_path.find_first_of(kPathDelimiterNonlocal, 0);
  while (cursor != std::string_view::npos) {
    str_path.replace(cursor, 1, 1, kPathDelimiter);
    cursor = str_path.find_first_of(kPathDelimiterNonlocal, cursor);
  }
  return str_path;
}

std::string CanonicalizePath(std::string_view path) {
  std::string str_path(path);
  auto cursor = str_path.find_first_of('\\', 0);
  while (cursor != std::string_view::npos) {
    str_path.replace(cursor, 1, 1, '/');
    cursor = str_path.find_first_of('\\', cursor);
  }
  return str_path;
}

}  // namespace redux
