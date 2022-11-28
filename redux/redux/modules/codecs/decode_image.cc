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

#include "redux/modules/codecs/decode_image.h"

#include "redux/modules/codecs/decode_stb.h"
#include "redux/modules/codecs/decode_webp.h"
#include "redux/modules/graphics/image_utils.h"

namespace redux {

ImageData DecodeImage(const DataContainer& data,
                      const DecodeImageOptions& opts) {
  switch (ImageFormat format = IdentifyImageTypeFromHeader(data.GetByteSpan());
          format) {
    case ImageFormat::Jpg:
    case ImageFormat::Png: {
      DecodeStbOptions stb_options;
      stb_options.premultiply_alpha = opts.premultiply_alpha;
      return DecodeStb(data, stb_options);
    }
    case ImageFormat::Webp: {
      DecodeWebpOptions webp_options;
      webp_options.premultiply_alpha = opts.premultiply_alpha;
      return DecodeWebp(data, webp_options);
    }
    default:
      LOG(FATAL) << "Undecodable format: " << ToString(format);
  }
}

}  // namespace redux
