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

#include "redux/tools/common/file_utils.h"

#if defined(_MSC_VER)
#include <direct.h>  // Windows functions for directory creation.
#else
#include <sys/stat.h>
#endif
#include <fstream>
#include <string>

#include "redux/modules/base/data_builder.h"

namespace redux::tool {

bool FileExists(const char* filename) {
  std::ifstream file(filename, std::ios::binary);
  return file ? true : false;
}

static int MakeDir(const char* sub_dir) {
#if defined(_MSC_VER)
  return _mkdir(sub_dir);
#else
  static const mode_t kDirectoryMode = 0755;
  return mkdir(sub_dir, kDirectoryMode);
#endif
}

bool CreateFolder(const char* directory) {
  if (directory == nullptr || *directory == 0) {
    return true;
  }

  std::string dir = directory;
  for (size_t slash = 0; slash != std::string::npos;) {
    // Find the next sub-directory (after the last one we just created).
    slash = dir.find_first_of("\\/", slash + 1);

    // If slash is npos, we take the entire `dir` and create it.
    const std::string sub_dir = dir.substr(0, slash);

    // Create the sub-directory (or continue if the directory already exists).
    const int result = MakeDir(sub_dir.c_str());
    if (result != 0 && errno != EEXIST) {
      return false;
    }
  }
  return true;
}

bool CopyFile(const char* dst, const char* src) {
  std::ifstream srcfile(src, std::ios::binary);
  std::ofstream dstfile(dst, std::ios::binary | std::ios::out);
  if (srcfile.is_open() && dstfile.is_open()) {
    dstfile << srcfile.rdbuf();
    return true;
  }
  return false;
}

absl::StatusOr<DataContainer> DefaultLoadFile(const char* filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    return absl::NotFoundError("Unable to open file.");
  }

  file.seekg(0, std::ios::end);
  const std::streamoff length = file.tellg();
  if (length < 0) {
    return absl::UnavailableError("Unable to determine file size.");
  }

  DataBuilder builder(length);
  std::byte* ptr = builder.GetAppendPtr(length);
  file.seekg(0, std::ios::beg);
  file.read(reinterpret_cast<char*>(ptr), length);
  return builder.Release();
}

static LoadFileFn g_load_file_fn = DefaultLoadFile;

absl::StatusOr<DataContainer> LoadFile(const char* filename) {
  return g_load_file_fn(filename);
}

std::string LoadFileAsString(const char* filename) {
  auto data = LoadFile(filename);
  CHECK(data.ok()) << "Unable to load file: " << filename;
  std::string str(reinterpret_cast<const char*>(data->GetBytes()),
                  data->GetNumBytes());
  if (!str.empty() && str.back() != '\n') {
    str.append("\n");
  }
  return str;
}

bool SaveFile(const void* bytes, size_t num_bytes, const char* filename,
              bool binary) {
  std::ofstream file(filename, binary ? std::ios::binary : std::ios::out);
  if (!file) {
    return false;
  }
  file.write(reinterpret_cast<const char*>(bytes), num_bytes);
  file.close();
  return true;
}

void SetLoadFileFunction(LoadFileFn fn) {
  if (fn) {
    g_load_file_fn = fn;
  } else {
    g_load_file_fn = DefaultLoadFile;
  }
}

}  // namespace redux::tool
