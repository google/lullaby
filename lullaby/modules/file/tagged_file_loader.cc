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

#include "lullaby/modules/file/tagged_file_loader.h"

#include <utility>

#include "ion/port/fileutils.h"
#include "lullaby/modules/file/file.h"
#include "lullaby/util/logging.h"

namespace lull {

namespace {

lull::TaggedFileLoader* g_tagged_loader = nullptr;

// Attempts to load using fplbase::LoadFileRaw if the filename is local,
// ion::port::ReadDataFromFile if the filename is absolute (starts with a '/').
bool LoadFileFallback(const char* filename, std::string* dest) {
  if (filename == nullptr) {
    return false;
  }
  if (filename[0] == '/') {
    return ion::port::ReadDataFromFile(filename, dest);
  } else {
    return fplbase::LoadFileRaw(filename, dest);
  }
}

}  // namespace

TaggedFileLoader::TaggedFileLoader() : fallback_load_fn_(LoadFileFallback) {}

void TaggedFileLoader::SetTaggedFileLoader(TaggedFileLoader* loader) {
  g_tagged_loader = loader;
}

bool TaggedFileLoader::LoadTaggedFile(const char* filename, std::string* dest) {
  if (!g_tagged_loader) {
    LOG(DFATAL) << "TaggedFileLoader not set";
    return false;
  }
  return g_tagged_loader->LoadFile(filename, dest);
}

bool TaggedFileLoader::ApplySettingsToTaggedFilename(
    const char* filename, std::string* const transformed_filename) {
  bool res = false;
  if (g_tagged_loader) {
    res = g_tagged_loader->ApplySettingsToFile(filename, transformed_filename);
  }
  if (!res && transformed_filename) {
    *transformed_filename = filename;
  }
  return res;
}

bool TaggedFileLoader::LoadFile(const char* filename, std::string* dest) {
  std::string transformed_filename;
  std::string tag_used;
  bool loaded = false;
  if (ApplySettingsToFile(filename, &transformed_filename, &tag_used)) {
    loaded = PlatformSpecificLoadFile(transformed_filename.c_str(), dest,
                                      tag_used);
    filename = transformed_filename.c_str();
  } else {
    loaded = fallback_load_fn_(filename, dest);
  }

  // Attempt to load from alternate file paths.
  if (!loaded) {
    std::string basename = GetBasenameFromFilename(filename);
    std::string extension = GetExtensionFromFilename(basename);
    auto iter = alt_paths_.find(extension);
    if (iter != alt_paths_.end()) {
      for (const std::string &path : iter->second) {
        loaded = fallback_load_fn_((path + basename).c_str(), dest);
        if (loaded) {
          return true;
        }
      }
    }
  }
  return loaded;
}

void TaggedFileLoader::RegisterTag(const std::string& tag,
                                   const std::string& path_prefix) {
  tag_settings_map_.emplace(tag, path_prefix);
}

void TaggedFileLoader::RegisterAltPathForSuffix(
    const std::string& suffix, const std::string& path) {
  alt_paths_[suffix].push_back(path);
}

void TaggedFileLoader::SetDefaultTag(const std::string& tag) {
  default_tag_ = tag;
}

TaggedFileLoader::LoadFileFn TaggedFileLoader::SetFallbackLoadFn(
    LoadFileFn fn) {
  if (!fn) {
    fn = fplbase::LoadFileRaw;
  }
  std::swap(fallback_load_fn_, fn);
  return fn;
}

bool TaggedFileLoader::ApplySettingsToFile(
    const char* filename, std::string* const transformed_filename) {
  return ApplySettingsToFile(filename, transformed_filename, nullptr);
}

bool TaggedFileLoader::ApplySettingsToFile(
    const char* filename, std::string* const transformed_filename,
    std::string* const tag_used) {
  auto replacement_file = replacement_map_.find(filename);
  if (replacement_file != replacement_map_.end()) {
    LOG(INFO) << "Replacing " << filename << " with "
              << replacement_file->second;
    filename = replacement_file->second.c_str();
  }
  const char* tag_delimiter = strchr(filename, ':');
  if (tag_delimiter != nullptr) {
    const std::string tag(filename, tag_delimiter - filename);
    filename = tag_delimiter + 1;
    if (tag_used) {
      *tag_used = tag;
    }
    return ApplySettingsToFile(tag, filename, transformed_filename);
  } else if (!default_tag_.empty()) {
    if (tag_used) {
      *tag_used = default_tag_;
    }
    return ApplySettingsToFile(default_tag_, filename, transformed_filename);
  } else {
    if (transformed_filename) {
      *transformed_filename = filename;
    }
    return false;
  }
}

bool TaggedFileLoader::ApplySettingsToFile(
    const std::string& tag, const char* filename,
    std::string* const transformed_filename) {
  const auto it = tag_settings_map_.find(tag);
  if (it == tag_settings_map_.end()) {
    LOG(WARNING) << "Unregistered tag " << tag << " in file " << filename;
    return false;
  }
  if (filename[0] == '/') {
    // Path is absolute, don't prepend anything.
    return false;
  }
  const std::string& path_prefix = it->second;
  if (transformed_filename) {
    if (!path_prefix.empty()) {
      *transformed_filename = path_prefix + filename;
    } else {
      *transformed_filename = filename;
    }
  }
  return true;
}

void TaggedFileLoader::AddReplacementFile(const std::string& from_file,
                                          const std::string& to_file) {
  replacement_map_.emplace(from_file, to_file);
}

}  // namespace lull
