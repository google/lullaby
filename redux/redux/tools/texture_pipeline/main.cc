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

#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "redux/modules/base/filepath.h"
#include "redux/modules/codecs/decode_image.h"
#include "redux/modules/codecs/encode_png.h"
#include "redux/modules/codecs/encode_webp.h"
#include "redux/tools/common/file_utils.h"
#include "redux/tools/texture_pipeline/generate_mipmaps.h"

ABSL_FLAG(std::vector<std::string>, input, std::vector<std::string>{},
          "Input image(s).");
ABSL_FLAG(std::string, output, "", "Output image.");
ABSL_FLAG(bool, generate_mipmaps, false, "Generates MipMap levels for image.");

namespace redux::tool {

static int RunTexturePipeline() {
  const std::vector<std::string> inputs = absl::GetFlag(FLAGS_input);
  CHECK(!inputs.empty()) << "Must specify input file.";

  const std::string output = absl::GetFlag(FLAGS_output);
  CHECK(!output.empty()) << "Must specify output file.";

  std::vector<ImageData> decoded_images;
  std::vector<DataContainer> loaded_inputs;
  for (const std::string& input : inputs) {
    absl::StatusOr<DataContainer> in = LoadFile(input.c_str());
    CHECK(in.ok()) << "Could not open input file: " << input;
    loaded_inputs.push_back(std::move(*in));

    ImageData image = DecodeImage(loaded_inputs.back(), {});
    CHECK(image.GetNumBytes() > 0) << "Unable to decode input file.";
    decoded_images.push_back(std::move(image));
  }

  if (absl::GetFlag(FLAGS_generate_mipmaps)) {
    CHECK_EQ(decoded_images.size(), 1)
        << "Can only generate mipmaps for a single image";
    decoded_images = GenerateMipmaps(std::move(decoded_images[0]));
  }

  DataContainer out;

  const std::string_view ext = GetExtension(output);
  if (ext == ".png") {
    CHECK_EQ(decoded_images.size(), 1) << "PNGs can only be a single image.";
    out = EncodePng(decoded_images[0]);
  } else if (ext == ".webp") {
    CHECK_EQ(decoded_images.size(), 1) << "WEBPs can only be a single image.";
    out = EncodeWebp(decoded_images[0]);
  } else {
    LOG(FATAL) << "Unknown format for exported file: " << ext;
  }

  CHECK(out.GetNumBytes() > 0) << "Unable to encode output image.";

  CHECK(SaveFile(out.GetBytes(), out.GetNumBytes(), output.c_str(), true))
      << "Failed to save to file: " << output;
  return 0;
}

}  // namespace redux::tool

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  return redux::tool::RunTexturePipeline();
}
