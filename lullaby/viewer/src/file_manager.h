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

#ifndef LULLABY_VIEWER_SRC_FILE_MANAGER_H_
#define LULLABY_VIEWER_SRC_FILE_MANAGER_H_

#include <unordered_map>
#include "lullaby/util/registry.h"

namespace lull {
namespace tool {

// Manages a "virtual" directory of files that the viewer.
//
// All load and save operations performed by the viewer are directed through
// the FileManager.  This allows the FileManager to load assets from
// various sources (eg. project directories, temporary directories, etc.) and
// create temporary files as needed.
//
// Internally, the FileManager uses mainly just the filename and discards the
// actual path of the file, so collisions are possible.
// TODO: Properly support directory structure.
class FileManager {
 public:
  explicit FileManager(Registry* registry);

  /// Returns true if the |filename| is known to the FileManager.
  bool Exists(const std::string& filename) const;

  /// Returns true if the |filename| is known to the FileManager.
  bool ExistsWithExtension(const std::string& filename) const;

  /// Returns the actual path of the file on the users computer.
  std::string GetRealPath(const std::string& filename) const;

  /// Loads the contents of the specified file into |out|.  Returns |true| if
  /// successful, false otherwise.
  bool LoadFile(const std::string& filename, std::string* out);

  /// Returns the list of all files with the specified |extension|.
  std::vector<std::string> FindAllFiles(const std::string extension) const;

  /// Registers a file with the FileManager.
  void ImportFile(const std::string& filename);

  /// Registers a directory with the FileManager, which recusively registers
  /// all files in all subfolders.
  void ImportDirectory(const std::string& filepath);

  /// Creates a temporary folder with the given prefix.
  static std::string MakeTempFolder(const std::string& prefix = "");

 private:
  using BuilderFn = std::function<bool(Registry* registry, const std::string&,
                                       const std::string&)>;

  bool BuildAsset(const std::string& target);

  Registry* registry_;
  std::set<std::string> names_;
  std::unordered_map<std::string, std::string> files_;
  std::unordered_map<std::string, BuilderFn> builders_;
};

}  // namespace tool
}  // namespace lull

LULLABY_SETUP_TYPEID(lull::tool::FileManager);

#endif  // LULLABY_VIEWER_SRC_FILE_MANAGER_H_
