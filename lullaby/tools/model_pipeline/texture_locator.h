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

#ifndef LULLABY_TOOLS_MODEL_PIPELINE_TEXTURE_LOCATOR_H_
#define LULLABY_TOOLS_MODEL_PIPELINE_TEXTURE_LOCATOR_H_

#include <memory>
#include <string>
#include <vector>

namespace lull {
namespace tool {

// Generates alternative names for use in a for-loop.
//
// A NameGenerator can be used to apply a set of transformations to a name which
// can then be evaluated in a loop.  For example:
//
//   for (size_t i = 0; gen.Valid(original_name, i); ++i) {
//     const std::string variation = gen.Apply(original_name, i);
//     Evaluate(variation);
//   }
//
// This is useful for things like testing different file extensions and changing
// between snake_case, CamelCase, lowercase, etc.
//
// These generators are used to perform an exhaustive search for textures.
struct NameGenerator {
  virtual ~NameGenerator() {}
  virtual bool Valid(const std::string& name, size_t index) const = 0;
  virtual std::string Apply(const std::string& name, size_t index) const = 0;
};

// Used to find texture resources on disk given a path encoded in a mesh file.
class TextureLocator {
 public:
  TextureLocator();

  // Sets an explicit path to a texture that can be used to look for files.
  void RegisterTexture(std::string name);

  // Sets an explicit path to a directory that may contain textures.
  void RegisterDirectory(std::string directory);

  // Finds the path of a file that best matches the requested texture.
  std::string FindTexture(const std::string& name) const;

 private:
  std::string Match(const std::string& name) const;

  std::vector<std::string> textures_;
  std::vector<std::string> directories_;
  std::vector<std::unique_ptr<NameGenerator>> generators_;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_MODEL_PIPELINE_TEXTURE_LOCATOR_H_
