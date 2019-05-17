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

#include "lullaby/util/arg_parser.h"
#include "lullaby/util/filename.h"
#include "lullaby/tools/common/file_utils.h"
#include "lullaby/tools/gltf_converter/gltf_converter.h"

namespace lull {
namespace tool {

static Span<uint8_t> ToSpan(const std::string& data) {
  return {reinterpret_cast<const uint8_t*>(data.data()), data.size()};
}

int Run(int argc, const char** argv) {
  ArgParser args;
  args.AddArg("gltf")
      .SetNumArgs(1)
      .SetRequired()
      .SetDescription("GLTF file to convert to GLB.");
  args.AddArg("outdir")
      .SetNumArgs(1)
      .SetDescription("Optional output directory.");

  // Parse the command-line arguments.
  if (!args.Parse(argc, argv)) {
    auto& errors = args.GetErrors();
    for (auto& err : errors) {
      std::cout << "Error: " << err << std::endl;
    }
    std::cout << args.GetUsage() << std::endl;
    return -1;
  }

  const std::string gltf(args.GetString("gltf"));

  std::string gltf_data;
  if (!LoadFile(gltf.c_str(), false, &gltf_data)) {
    std::cout << "Could not load GLTF file: " << gltf << std::endl;
    return -1;
  }

  const std::string dir = GetDirectoryFromFilename(gltf);
  auto load_fn = [&](string_view filename) -> ByteArray {
    std::string path = JoinPath(dir, filename);
    std::string data;
    if (!LoadFile(path.c_str(), false, &data)) {
      std::cout << "Could not load file: " << filename << std::endl;
      return {};
    }
    ByteArray res{data.begin(), data.end()};
    return res;
  };

  const ByteArray glb = GltfToGlb(ToSpan(gltf_data), load_fn);
  if (glb.empty()) {
    std::cout << "Could not convert GLTF file: " << gltf << std::endl;
    return -1;
  }

  std::string output(args.GetString("output"));
  if (output.empty()) {
    output = RemoveExtensionFromFilename(gltf) + ".glb";
  }

  if (!SaveFile(glb.data(), glb.size(), output.c_str(), true)) {
    std::cout << "Unable to save glb:" << output << std::endl;
    return -1;
  }

  return 0;
}

}  // namespace tool
}  // namespace lull

int main(int argc, const char** argv) { return lull::tool::Run(argc, argv); }
