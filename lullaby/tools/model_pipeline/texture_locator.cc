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

#include "lullaby/tools/model_pipeline/texture_locator.h"

#include <algorithm>
#include <functional>
#include "lullaby/util/filename.h"
#include "lullaby/util/logging.h"
#include "lullaby/tools/common/file_utils.h"

namespace lull {
namespace tool {

static inline bool IsSpace(char c) {
  return c == ' ' || c == '_';
}

static inline bool IsDir(char c) {
  return c == '/' || c == '\\';
}

static char LastChar(const std::string& str) {
  return str.empty() ? 0 : str.back();
}

static std::string NoSpaces(const std::string& str) {
  std::string result;
  result.reserve(str.size());
  for (char c : str) {
    if (c == ' ') {
      result += '_';
    } else {
      result += c;
    }
  }
  return result;
}

static std::string SnakeCase(const std::string& str) {
  std::string result;
  result.reserve(2 * str.size());

  char prev = 0;
  for (char c : str) {
    const char last = LastChar(result);
    if (IsSpace(c)) {
      if (!IsSpace(last) && !IsDir(last)) {
        result += '_';
      }
    } else if (isupper(c)) {
      if (!IsSpace(last) && !IsDir(last) && !isupper(prev)) {
        result += '_';
      }
      result += static_cast<char>(tolower(c));
    } else {
      result += c;
    }
    prev = c;
  }
  if (LastChar(result) == '_') {
    result.resize(result.size() - 1);
  }
  return result;
}

static std::string CamelCase(const std::string& str) {
  std::string result;
  result.reserve(str.size());

  bool capitalize_next = true;
  for (char c : str) {
    if (IsSpace(c)) {
      capitalize_next = true;
      continue;
    } else if (IsDir(c)) {
      capitalize_next = true;
      result += c;
      continue;
    }

    if (capitalize_next) {
      result += static_cast<char>(toupper(c));
    } else {
      result += c;
    }
    capitalize_next = false;
  }
  return result;
}

// A simple iterator adapter around the NameGenerator class.
class Iter {
 public:
  Iter(std::string name, const NameGenerator* generator)
      : name_(std::move(name)), generator_(generator) {}

  void operator++() { ++stage_; }
  explicit operator bool() const { return generator_->Valid(name_, stage_); }
  std::string operator*() const { return generator_->Apply(name_, stage_); }

 private:
  std::string name_;
  size_t stage_ = 0;
  const NameGenerator* generator_ = nullptr;
};

// A NameGenerator that applies different casing rules (eg. snake_case,
// CamelCase, lowercase, etc.).
class CaseGenerator : public NameGenerator {
 public:
  CaseGenerator() {
    ops_.emplace_back([](const std::string& name) { return name; });
    ops_.emplace_back(NoSpaces);
    ops_.emplace_back(SnakeCase);
    ops_.emplace_back(CamelCase);
  }

  bool Valid(const std::string& name, size_t index) const override {
    return index < ops_.size();
  }

  std::string Apply(const std::string& name, size_t index) const override {
    return ops_[index](name);
  }

 private:
  using Op = std::function<std::string(const std::string&)>;
  std::vector<Op> ops_;
};

static const char* kImageExtensions[] = {".jpg", ".jpeg", ".webp", ".png",
                                         ".tga", ".astc", ".ktx", ".rgbm"};
static const size_t kNumImageExtensions =
    sizeof(kImageExtensions) / sizeof(kImageExtensions[0]);

// A NameGenerator that applies different file extensions.
class ExtensionGenerator : public NameGenerator {
 public:
  bool Valid(const std::string& name, size_t index) const override {
    return index < kNumImageExtensions;
  }

  std::string Apply(const std::string& name, size_t index) const override {
    const char* ext = kImageExtensions[index];
    return RemoveExtensionFromFilename(name) + ext;
  }
};

// A NameGenerator that incrementally strips off directories from a path. For
// example, the path "a/b/image.gif" would be evaluated as:
//   a/b/image.gif
//   b/image.gif
//   image.gif
class DirStripGenerator : public NameGenerator {
 public:
  bool Valid(const std::string& name, size_t index) const override {
    const size_t num_breaks = std::count(name.begin(), name.end(), '/') +
                              std::count(name.begin(), name.end(), '\\');
    return index <= num_breaks;
  }

  std::string Apply(const std::string& name, size_t index) const override {
    std::string tmp = name;
    for (size_t i = 0; i < index; ++i) {
      auto pos1 = tmp.find('/');
      auto pos2 = tmp.find('\\');
      if (pos1 != std::string::npos && pos2 != std::string::npos) {
        tmp = tmp.substr(std::min(pos1, pos2) + 1);
      } else if (pos1 != std::string::npos) {
        tmp = tmp.substr(pos1 + 1);
      } else if (pos2 != std::string::npos) {
        tmp = tmp.substr(pos2 + 1);
      }
    }
    return tmp;
  }
};

TextureLocator::TextureLocator() {
  generators_.emplace_back(new CaseGenerator());
  generators_.emplace_back(new ExtensionGenerator());
  generators_.emplace_back(new DirStripGenerator());
  RegisterDirectory(".");
}

void TextureLocator::RegisterTexture(std::string name) {
  textures_.push_back(std::move(name));
}

void TextureLocator::RegisterDirectory(std::string directory) {
  directories_.push_back(std::move(directory));
}

std::string TextureLocator::Match(const std::string& name) const {
  const std::string basename = RemoveDirectoryAndExtensionFromFilename(name);
  for (const std::string& texture : textures_) {
    if (basename == RemoveDirectoryAndExtensionFromFilename(texture)) {
      return texture;
    }
  }
  if (FileExists(name.c_str())) {
    return name;
  }
  return "";
}

std::string TextureLocator::FindTexture(const std::string& name) const {
  for (Iter iter1(name, generators_[0].get()); iter1; ++iter1) {
    for (Iter iter2(*iter1, generators_[1].get()); iter2; ++iter2) {
      for (Iter iter3(*iter2, generators_[2].get()); iter3; ++iter3) {
        for (auto& directory : directories_) {
          std::string test_filename = JoinPath(directory, *iter3);
          const std::string res = Match(test_filename);
          if (!res.empty()) {
            return res;
          }
        }
      }
    }
  }
  return Match(name);
}

}  // namespace tool
}  // namespace lull
