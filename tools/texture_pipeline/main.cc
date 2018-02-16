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

#include "lullaby/modules/render/image_data.h"
#include "lullaby/modules/render/image_decode.h"
#include "lullaby/util/arg_parser.h"
#include "lullaby/util/common_types.h"
#include "lullaby/util/filename.h"
#include "lullaby/util/logging.h"
#include "tools/common/file_utils.h"
#include "tools/texture_pipeline/encode_astc.h"
#include "tools/texture_pipeline/encode_jpg.h"
#include "tools/texture_pipeline/encode_ktx.h"
#include "tools/texture_pipeline/encode_png.h"
#include "tools/texture_pipeline/encode_webp.h"

namespace lull {
namespace tool {

int Run(int argc, const char** argv) {
  ArgParser parser;
  parser.AddArg("in").SetNumArgs(1).SetRequired();
  parser.AddArg("out").SetNumArgs(1).SetRequired();

  if (!parser.Parse(argc, argv)) {
    LOG(ERROR) << "Failed to parse args:";
    const auto& errors = parser.GetErrors();
    for (const auto& error : errors) {
      LOG(ERROR) << error;
    }
    return -1;
  }

  const std::string input = parser.GetString("in").to_string();
  const std::string output = parser.GetString("out").to_string();

  std::string src;
  if (!LoadFile(input.c_str(), true, &src)) {
    LOG(ERROR) << "Unable to load file: " << input;
    return -1;
  }

  ImageData image = DecodeImage(reinterpret_cast<const uint8_t*>(src.data()),
                                src.size(), kDecodeImage_None);
  if (image.IsEmpty()) {
    LOG(ERROR) << "Unable to decode file: " << input;
    return -1;
  }

  const std::string ext = GetExtensionFromFilename(output);

  ByteArray new_image;
  if (ext == ".webp") {
    new_image = EncodeWebp(image);
  } else if (ext == ".png") {
    new_image = EncodePng(image);
  } else if (ext == ".jpg") {
    new_image = EncodeJpg(image);
  } else if (ext == ".astc") {
    new_image = EncodeAstc(image);
  } else if (ext == ".ktx") {
    new_image = EncodeKtx(image);
  } else {
    LOG(ERROR) << "Unsupported output format: " << output;
    LOG(ERROR) << "Must be: webp, png, jpg, astc, or ktx (etc2).";
    return -1;
  }

  if (new_image.empty()) {
    LOG(ERROR) << "Unable to re-encode image: " << output;
    return -1;
  }

  if (!SaveFile(new_image.data(), new_image.size(), output.c_str(), true)) {
    LOG(FATAL) << "Failed to save new image: " << output;
    return -1;
  }
  return 0;
}

}  // namespace tool
}  // namespace lull

int main(int argc, const char** argv) { return lull::tool::Run(argc, argv); }
