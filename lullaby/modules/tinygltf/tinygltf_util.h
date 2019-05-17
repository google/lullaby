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

#ifndef LULLABY_MODULES_TINYGLTF_TINYGLTF_UTIL_H_
#define LULLABY_MODULES_TINYGLTF_TINYGLTF_UTIL_H_

#include "lullaby/util/optional.h"
#include "motive/matrix_anim.h"
#include "motive/matrix_op.h"
#include "tiny_gltf.h"

namespace lull {

/// Indicates an empty or invalid property in TinyGLTF, such as a Node
/// indicating it has no Mesh.
static constexpr int kInvalidTinyGltfIndex = -1;

/// Fetches a raw pointer of type T for |accessor| using the buffer views and
/// buffers in |model|.
template <typename T>
const T* DataFromGltfAccessor(const tinygltf::Model& model,
                              const tinygltf::Accessor& accessor) {
  if (accessor.bufferView == kInvalidTinyGltfIndex) {
    return nullptr;
  }
  const tinygltf::BufferView& buffer_view =
      model.bufferViews[accessor.bufferView];
  const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];
  return reinterpret_cast<const T*>(
      &buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
}

/// Returns the size of an element of |accessor| in bytes.
size_t ElementSizeInBytes(const tinygltf::Accessor& accessor);

/// Fetches the byte stride, which is the number of bytes each element takes up,
/// for |accessor| using the buffer views in |model|. If no byte stride is
/// specified by the buffer view, assumes the buffer is tightly packed and
/// returns the size of an element in |accessor|.
size_t ByteStrideFromGltfAccessor(const tinygltf::Model& model,
                                  const tinygltf::Accessor& accessor);

Optional<size_t> GetChannelCount(const tinygltf::AnimationSampler& sampler,
                                 const tinygltf::Model& model);

/// Simple struct containing all TinyGLTF data related to the animations of a
/// particular TinyGLTF Node.
struct TinyGltfNodeAnimationData {
  TinyGltfNodeAnimationData(const tinygltf::Node& node,
                            const tinygltf::Model& model)
      : node(node), model(model) {}

  /// Assigns a TinyGLTF |channel| from a TinyGLTF |animation| to a property of
  /// this Node animation based on it's target path.
  bool SetChannel(const tinygltf::Animation* animation,
                  const tinygltf::AnimationChannel* channel) {
    if (channel->target_path == "translation") {
      translation = &animation->samplers[channel->sampler];
    } else if (channel->target_path == "rotation") {
      rotation = &animation->samplers[channel->sampler];
    } else if (channel->target_path == "scale") {
      scale = &animation->samplers[channel->sampler];
    } else if (channel->target_path == "weights") {
      const tinygltf::AnimationSampler& sampler =
          animation->samplers[channel->sampler];
      Optional<size_t> channel_count = GetChannelCount(sampler, model);
      if (!channel_count) {
        return false;
      }
      weights_channel_count = *channel_count;
      weights = &sampler;
    } else {
      return false;
    }
    return true;
  }

  size_t GetRequiredSplineCount() const {
    size_t spline_count = 0;
    if (translation != nullptr) {
      spline_count += 3;
    }
    if (rotation != nullptr) {
      spline_count += 4;
    }
    if (scale != nullptr) {
      spline_count += 3;
    }
    spline_count += weights_channel_count;
    return spline_count;
  }

  bool HasTranslation() const { return translation != nullptr; }
  bool HasRotation() const { return rotation != nullptr; }
  bool HasScale() const { return scale != nullptr; }
  bool HasWeights() const { return weights != nullptr; }

  const tinygltf::Node& node;
  const tinygltf::Model& model;
  const tinygltf::AnimationSampler* translation = nullptr;
  const tinygltf::AnimationSampler* rotation = nullptr;
  const tinygltf::AnimationSampler* scale = nullptr;
  const tinygltf::AnimationSampler* weights = nullptr;
  size_t weights_channel_count = 0;
};

/// Returns the number of bytes required to represent |data| or NullOpt if the
/// number of bytes cannot be determined.
Optional<size_t> GetRequiredBufferSize(const TinyGltfNodeAnimationData& data);

/// Creates CompactSplines from |data| into a caller-allocated |buffer|. The
/// first CompactSpline will be at the beginning of the buffer, and subsequent
/// CompactSplines can be accessed using CompactSpline::Next().
///
/// Note that this function does no bounds checking. Use GetRequiredBufferSize()
/// to create a buffer large enough.
Optional<size_t> AddAnimationData(uint8_t* buffer,
                                  const TinyGltfNodeAnimationData& data);

/// Populates |matrix_anim| with animation |data|. This include allocating
/// splines, populating them with curve data, and creating the matrix operations
/// that drive the animation.
bool AddAnimationData(motive::MatrixAnim* matrix_anim,
                      const TinyGltfNodeAnimationData& data);

}  // namespace lull

#endif  // LULLABY_MODULES_TINYGLTF_TINYGLTF_UTIL_H_
