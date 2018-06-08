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

#include <iostream>

#include "lullaby/util/arg_parser.h"
#include "lullaby/util/filename.h"
#include "lullaby/tools/pack_ktx/ktx_astc_image.h"
#include "lullaby/tools/pack_ktx/ktx_direct_image.h"
#include "lullaby/tools/pack_ktx/ktx_image.h"

using lull::ArgParser;
using lull::tool::KtxAstcImage;
using lull::tool::KtxCubemapImage;
using lull::tool::KtxDirectImage;
using lull::tool::KtxImage;
using lull::tool::KtxMipmapImage;

int main(int argc, const char** argv) {
  ArgParser args;
  args.AddArg("cube-map")
      .SetShortName('c')
      .SetNumArgs(0)
      .SetDescription("The input files represent the faces of a cube map.");
  args.AddArg("output")
      .SetShortName('o')
      .SetRequired()
      .SetNumArgs(1)
      .SetDescription("The output file name.");
  args.AddArg("extract-image-index")
      .SetShortName('e')
      .SetNumArgs(1)
      .SetDescription(
          "The index of the image to extract from a KTX source file.");
  args.AddArg("drop-mip-levels")
      .SetNumArgs(1)
      .SetShortName('d')
      .SetDescription("The number of mip levels to drop.");

  // Parse the command-line arguments.
  if ((!args.Parse(argc, argv)) || (args.GetPositionalArgs().empty())) {
    auto& errors = args.GetErrors();
    for (auto& err : errors) {
      std::cout << "Error: " << err << std::endl;
    }
    std::cout << args.GetUsage() << std::endl;
    std::cout << "Supported source formats:" << std::endl
              << "  astc - "
                 "https://www.khronos.org/opengl/wiki/ASTC_Texture_Compression"
              << std::endl;
    return -1;
  }

  bool cube_map = args.IsSet("cube-map");
  int extract_image = -1;
  if (args.IsSet("extract-image-index")) {
    extract_image = args.GetInt("extract-image-index", 0);
  }
  int drop_mip_levels = 0;
  if (args.IsSet("drop-mip-levels")) {
    drop_mip_levels = args.GetInt("drop-mip-levels", 0);
  }
  // TODO(gavindodd): switch std::string over to lull::string_view wherever
  // possible
  std::vector<std::string> source_files;
  for (const auto& arg : args.GetPositionalArgs()) {
    source_files.push_back(arg.c_str());
  }
  std::string output_file = args.GetString("output", 0).c_str();
  KtxImage::ImagePtr img;
  if (args.IsSet("drop-mip-levels")) {
    if (source_files.size() > 1) {
      std::cerr << "Only one source file is expected for extract image\n";
      return -1;
    }
    if (cube_map) {
      std::cerr << "--cube-map is incompatible with --drop-mip-levels\n";
      return -1;
    }
    auto result = KtxDirectImage::Open(source_files[0], &img);
    if ((result != KtxImage::kOK) || (img == nullptr)) {
      std::cerr << "Could not open " << source_files[0]
                << " as valid KTX file\n";
      return -1;
    }
    KtxDirectImage* direct_img = static_cast<KtxDirectImage*>(img.get());
    direct_img->DropMips(drop_mip_levels);
    if (!direct_img->WriteFile(output_file)) {
      std::cerr << "Could not write KTX image\n";
      return -1;
    }
    return 0;
  } else if (extract_image >= 0) {
    if (source_files.size() > 1) {
      std::cerr << "Only one source file is expected for extract image\n";
      return -1;
    }
    if (cube_map) {
      std::cerr << "--cube-map is incompatible with --extract-image-index\n";
      return -1;
    }
    KtxImage::ErrorCode result = KtxDirectImage::Open(source_files[0], &img);
    if ((result != KtxImage::kOK) || (img == nullptr)) {
      std::cerr << "Could not open image as valid KTX file\n";
      return -1;
    }
    std::cout << "Extracting image\n";
    result = KtxAstcImage::WriteASTC(*img, extract_image, output_file);
    if (result != KtxImage::kOK) {
      std::cerr << "Could not write image as ASTC file\n";
      return -1;
    }
    std::cout << "Success.\n";
    return 0;
  } else if (cube_map) {
    if (source_files.size() % 6 != 0) {
      std::cerr << "Must have a multiple of 6 source files for cube map\n";
      return -1;
    }
    if (source_files.size() == 6) {
      KtxImage::ErrorCode result =
          KtxCubemapImage::Open(source_files, KtxAstcImage::Open, &img);
      if ((result != KtxImage::kOK) || (img == nullptr)) {
        std::cerr << "Could not open images as valid astc cube map\n";
        return -1;
      }
    } else {
      KtxImage::ErrorCode result =
          KtxMipmapImage::OpenCubemap(source_files, KtxAstcImage::Open, &img);
      if ((result != KtxImage::kOK) || (img == nullptr)) {
        std::cerr << "Could not open images as valid astc cube map\n";
        return -1;
      }
    }
  } else if (source_files.size() == 1) {
    KtxImage::ErrorCode result = KtxAstcImage::Open(source_files[0], &img);
    if ((result != KtxImage::kOK) || (img == nullptr)) {
      std::cerr << "Could not open " << source_files[0] << " as astc\n";
      return -1;
    }
  } else {
    KtxImage::ErrorCode result =
        KtxMipmapImage::Open(source_files, KtxAstcImage::Open, &img);
    if ((result != KtxImage::kOK) || (img == nullptr)) {
      std::cerr << "Could not open images as valid astc mipmaps\n";
      return -1;
    }
  }
  if (!img->WriteFile(output_file)) {
    std::cerr << "Could not write KTX image\n";
    return -1;
  }
  std::cout << "Success.\n";
  return 0;
}
