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

#include "lullaby/viewer/src/file_manager.h"

#include <dirent.h>
#include <sys/stat.h>

#include "flatbuffers/util.h"
#include "fplbase/utilities.h"
#include "lullaby/util/filename.h"
#include "lullaby/tools/common/file_utils.h"
#include "lullaby/viewer/src/builders/build_blueprint.h"
#include "lullaby/viewer/src/builders/build_model.h"
#include "lullaby/viewer/src/builders/build_rig_animation.h"
#include "lullaby/viewer/src/builders/build_shader.h"
#include "lullaby/viewer/src/builders/build_shading_model.h"
#include "lullaby/viewer/src/builders/build_stategraph.h"
#include "lullaby/viewer/src/widgets/file_dialog.h"

const char* kWorkspaceDirectory = "/tmp/lullaby_viewer/";
const char* kTempDirectory = "/tmp/lullaby_viewer_tmp/";

namespace lull {
namespace tool {

FileManager* g_file_manager = nullptr;

bool FplLoadFileFunction(const char* filename, std::string* out) {
  return g_file_manager->LoadFile(filename, out);
}
bool FlatbuffersLoadFileFunction(const char* filename, bool binary,
                                 std::string* out) {
  return g_file_manager->LoadFile(filename, out);
}
bool FlatbuffersFileExistsFunction(const char* filename) {
  return g_file_manager->Exists(filename);
}
bool FileUtilsLoadFileFunction(const char* filename, bool binary,
                               std::string* out) {
  return g_file_manager->LoadFile(filename, out);
}

FileManager::FileManager(Registry* registry) : registry_(registry) {
  g_file_manager = this;
  fplbase::SetLoadFileFunction(FplLoadFileFunction);
  flatbuffers::SetLoadFileFunction(FlatbuffersLoadFileFunction);
  flatbuffers::SetFileExistsFunction(FlatbuffersFileExistsFunction);
  SetLoadFileFunction(FileUtilsLoadFileFunction);

  ImportDirectory(".");

  builders_[".bin"] = BuildBlueprint;
  builders_[".lullmodel"] = BuildModel;
  builders_[".fplshader"] = BuildShader;
  builders_[".lullshader"] = BuildShadingModel;
  builders_[".motiveanim"] = BuildRigAnimation;
  builders_[".stategraph"] = BuildStategraph;
}

bool FileManager::BuildAsset(const std::string& target) {
  const std::string ext = GetExtensionFromFilename(target);
  auto iter = builders_.find(ext);
  if (iter == builders_.end()) {
    return false;
  }

  const std::string name = RemoveDirectoryAndExtensionFromFilename(target);
  const std::string out_dir = MakeTempFolder(name);
  const bool result = iter->second(registry_, target, out_dir);
  if (result) {
    ImportDirectory(out_dir);
  }
  return result;
}

bool FileManager::Exists(const std::string& filename) const {
  const std::string name = RemoveExtensionFromFilename(filename);
  return names_.count(name) > 0;
}

bool FileManager::ExistsWithExtension(const std::string& filename) const {
  const std::string basename = GetBasenameFromFilename(filename);
  return files_.count(basename) > 0;
}

std::string FileManager::GetRealPath(const std::string& filename) const {
  const std::string basename = GetBasenameFromFilename(filename);
  auto iter = files_.find(basename);
  return iter != files_.end() ? iter->second : std::string("");
}

std::vector<std::string> FileManager::FindAllFiles(
    const std::string extension) const {
  std::vector<std::string> res;
  for (auto& iter : files_) {
    if (GetExtensionFromFilename(iter.first) == extension) {
      res.push_back(iter.first);
    }
  }
  return res;
}

bool FileManager::LoadFile(const std::string& filename, std::string* out) {
  bool status = false;

  const std::string basename = GetBasenameFromFilename(filename);
  auto iter = files_.find(basename);
  if (iter != files_.end()) {
    status = fplbase::LoadFileRaw(iter->second.c_str(), out);
  }
  if (status == false) {
    status = fplbase::LoadFileRaw(filename.c_str(), out);
  }
  if (status == false) {
    const bool fallback = BuildAsset(filename);
    if (fallback) {
      iter = files_.find(basename);
      if (iter != files_.end()) {
        status = fplbase::LoadFileRaw(iter->second.c_str(), out);
      }
    }
  }
  return status;
}

void FileManager::ImportFile(const std::string& filename) {
  const std::string name = GetBasenameFromFilename(filename);

  auto iter = files_.find(name);
  if (iter != files_.end() && iter->second != filename) {
    LOG(ERROR) << "Overwriting " << iter->second << " with " << filename;
  }

  names_.insert(RemoveExtensionFromFilename(name));
  files_[name] = filename;
}

void FileManager::ImportDirectory(const std::string& filepath) {
  DIR* dir = opendir(filepath.c_str());
  if (dir == nullptr) {
    return;
  }

  while (dirent* entry = readdir(dir)) {
    if (entry->d_name[0] == '.') {
      continue;
    }

    const std::string fullpath = filepath + "/" + entry->d_name;
    struct stat st;
    stat(fullpath.c_str(), &st);

    if (st.st_mode & S_IFDIR) {
      ImportDirectory(fullpath);
    } else {
      ImportFile(fullpath);
    }
  }
  closedir(dir);
}

std::string FileManager::MakeTempFolder(const std::string& prefix) {
  std::stringstream ss;
  ss << kTempDirectory << "/" << prefix << "."
     << std::chrono::steady_clock::now().time_since_epoch().count();

  const std::string path = ss.str();
  CreateFolder(ss.str().c_str());
  return path + "/";
}

}  // namespace tool
}  // namespace lull
