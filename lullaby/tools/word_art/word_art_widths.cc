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

#include <iostream>
#include <map>

#include "lullaby/generated/flatbuffers/model_def_generated.h"
#include "lullaby/util/arg_parser.h"
#include "lullaby/util/filename.h"
#include "lullaby/tools/common/file_utils.h"

int main(int argc, const char* argv[]) {
  lull::ArgParser args;
  // Parse the command-line arguments.
  if (!args.Parse(argc, argv)) {
    auto& errors = args.GetErrors();
    for (auto& err : errors) {
      std::cout << "Error: " << err << std::endl;
    }
    std::cout << args.GetUsage() << std::endl;
    return -1;
  }

  std::map<std::string, float> glyph_widths;
  const auto& filenames = args.GetPositionalArgs();
  for (int i = 0; i < filenames.size(); ++i) {
    const std::string arg(filenames[i]);
    std::string data;
    if (!lull::tool::LoadFile(arg.c_str(), true, &data)) {
      std::cout << "Error: failed to load " << arg << std::endl;
      return -1;
    }
    auto* model_def = lull::GetModelDef(data.data());
    const auto* aabb = model_def->bounding_box();
    if (!aabb) {
      std::cout << "Error: model " << arg << " has no bounding box.";
      return -1;
    }
    glyph_widths[lull::GetBasenameFromFilename(arg)] =
        aabb->max().x() - aabb->min().x();
  }

  std::cout << "[" << std::endl;
  for (const auto& kv : glyph_widths) {
    std::cout << "  {\"glyph\": \"" << kv.first
              << "\", \"width\": " << kv.second << "}," << std::endl;
  }
  std::cout << "]" << std::endl;
  return 0;
}
