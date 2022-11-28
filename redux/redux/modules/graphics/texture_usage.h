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

#ifndef REDUX_MODULES_GRAPHICS_TEXTURE_USAGE_H_
#define REDUX_MODULES_GRAPHICS_TEXTURE_USAGE_H_

#include <cstddef>
#include <initializer_list>

#include "absl/types/span.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/graphics/enums.h"

namespace redux {

// Associates a MaterialTextureType to each channel of a texture.
struct TextureUsage {
  TextureUsage() = default;

  TextureUsage(std::initializer_list<MaterialTextureType> types) {
    CHECK_GT(types.size(), 0);
    CHECK_LE(types.size(), kNumChannels);
    for (std::size_t i = 0; i < types.size(); ++i) {
      channel[i] = types.begin()[i];
    }
    for (std::size_t i = types.size() - 1; i < kNumChannels; ++i) {
      channel[i] = channel[types.size() - 1];
    }
  }

  template <typename T>
  explicit TextureUsage(const T& types) {
    if constexpr (std::is_same_v<T, MaterialTextureType>) {
      for (std::size_t i = 0; i < kNumChannels; ++i) {
        channel[i] = types;
      }
    } else {
      CHECK_GT(types.size(), 0);
      CHECK_LE(types.size(), kNumChannels);
      for (std::size_t i = 0; i < types.size(); ++i) {
        channel[i] = static_cast<MaterialTextureType>(types[i]);
      }
      for (std::size_t i = types.size() - 1; i < kNumChannels; ++i) {
        channel[i] = channel[types.size() - 1];
      }
    }
  }

  // Combines two TextureUsages such that, for a given channel, the usage will
  // come from either inputs. An error will occur if both inputs are using the
  // same channel. A channel will remain unused if neither inputs is using the
  // channel.
  static TextureUsage Combine(TextureUsage lhs, TextureUsage rhs) {
    TextureUsage out;
    for (std::size_t i = 0; i < kNumChannels; ++i) {
      if (lhs.channel[i] == rhs.channel[i]) {
        out.channel[i] = lhs.channel[i];
      } else if (lhs.channel[i] == MaterialTextureType::Unspecified) {
        out.channel[i] = rhs.channel[i];
      } else if (rhs.channel[i] == MaterialTextureType::Unspecified) {
        out.channel[i] = lhs.channel[i];
      } else {
        LOG(FATAL) << "Unable to combine texture usages.";
      }
    }
    return out;
  }

  // Determines if `this` has the same texture usage per channel as `other`.
  // Unused channels of other are ignored.
  bool IsSupersetOf(const TextureUsage& other) const {
    for (std::size_t i = 0; i < kNumChannels; ++i) {
      const MaterialTextureType lhs = channel[i];
      const MaterialTextureType rhs = other.channel[i];

      if (lhs != rhs && rhs != MaterialTextureType::Unspecified) {
        return false;
      }
    }
    return true;
  }

  // Determines if `this` has the same texture usage per channel as `other`.
  // Unused channels of `this` are ignored.
  bool IsSubsetOf(const TextureUsage& other) const {
    return other.IsSupersetOf(*this);
  }

  // Generates a Hash using the TextureUsages for each channel.
  HashValue Hash() const {
    const HashValue a = redux::Hash(ToString(channel[0]));
    const HashValue b = redux::Hash(ToString(channel[1]));
    const HashValue c = redux::Hash(ToString(channel[2]));
    const HashValue d = redux::Hash(ToString(channel[3]));
    return redux::Combine(redux::Combine(redux::Combine(a, b), c), d);
  }

  friend bool operator==(const TextureUsage& lhs, const TextureUsage& rhs) {
    return lhs.channel[0] == rhs.channel[0] &&
           lhs.channel[1] == rhs.channel[1] &&
           lhs.channel[2] == rhs.channel[2] && lhs.channel[3] == rhs.channel[3];
  }

  template <typename H>
  friend H AbslHashValue(H h, const TextureUsage& usage) {
    return H::combine(std::move(h), usage.channel[0], usage.channel[1],
                      usage.channel[2], usage.channel[3]);
  }

  static constexpr int kNumChannels = 4;

  MaterialTextureType channel[kNumChannels] = {
      MaterialTextureType::Unspecified, MaterialTextureType::Unspecified,
      MaterialTextureType::Unspecified, MaterialTextureType::Unspecified};
};

}  // namespace redux

#endif  // REDUX_MODULES_GRAPHICS_TEXTURE_USAGE_H_
