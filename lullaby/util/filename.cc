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

#include "lullaby/util/filename.h"

namespace lull {

bool EndsWith(const std::string& filename, const std::string& suffix) {
  return filename.size() >= suffix.size() &&
         filename.compare(
             filename.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string GetBasenameFromFilename(const std::string& filename) {
  const size_t index = filename.find_last_of("/\\");
  if (index == std::string::npos) {
    return filename;
  }
  if (index - 1 == filename.length()) {
    return "";
  }
  return filename.substr(index + 1);
}

std::string GetExtensionFromFilename(const std::string& filename) {
  const size_t index = filename.rfind('.');
  if (index == std::string::npos) {
    return "";
  }
  return filename.substr(index);
}

std::string RemoveExtensionFromFilename(const std::string& filename) {
  const size_t index = filename.rfind('.');
  if (index == std::string::npos) {
    return filename;
  }
  return filename.substr(0, index);
}

std::string RemoveDirectoryAndExtensionFromFilename(
    const std::string& filename) {
  return GetBasenameFromFilename(RemoveExtensionFromFilename(filename));
}

std::string GetDirectoryFromFilename(const std::string& filename) {
  const size_t index = filename.find_last_of("/\\");
  if (index == std::string::npos) {
    return "";
  }
  return filename.substr(0, index);
}

std::string JoinPath(const std::string& directory,
                     const std::string& basename) {
#if defined(_WINDOWS) || defined(_WIN32)
  const char kPathDelimiter = '\\';
#else
  const char kPathDelimiter = '/';
#endif  // defined(_WINDOWS) || defined(_WIN32)

  // Ensure directory does not have a trailing slash.
  std::string cleaned_directory =
      (!directory.empty() &&
       directory.find_first_of("/\\", directory.length() - 1) !=
           std::string::npos)
          ? directory.substr(0, directory.length() - 1)
          : directory;

  // Ensure basename does not have a leading slash (unless dirname is empty,
  // in which case we treat basename as a full path).
  std::string cleaned_basename =
      (!basename.empty() && !directory.empty() &&
       basename.find_last_of("/\\", 0) != std::string::npos)
      ? basename.substr(1, std::string::npos) : basename;

  // Combine the cleaned directory and base names.  For consistency, we emit
  // local paths (e.g. './foo.txt') without the leading './'.
  return (cleaned_directory == "." || cleaned_directory.empty())
             ? cleaned_basename
             : cleaned_directory + kPathDelimiter + cleaned_basename;
}

}  // namespace lull
