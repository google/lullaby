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

#include "lullaby/modules/tinygltf/tinygltf_util.h"

#include "lullaby/util/span.h"
#include "motive/util/keyframe_converter.h"

namespace lull {

namespace {

motive::MatrixOpId MatrixOpBaseIdFromBaseType(
    motive::MatrixOperationType base_type) {
  switch (base_type) {
    case motive::kTranslateX:
      return 0;
    case motive::kQuaternionW:
      return 3;
    case motive::kScaleX:
      return 7;
    default:
      return -1;
  }
}

/// Returns the Motive InterpolationType for a given |type|, which must match
/// one of the values of tinygltf::AnimationSampler::interpolation, or NullOpt
/// if there is no match.
Optional<motive::InterpolationType> InterpolationTypeForString(
    const std::string& type) {
  if (type == "LINEAR") {
    return motive::kLinear;
  } else if (type == "STEP") {
    return motive::kStep;
  } else if (type == "CUBICSPLINE") {
    return motive::kCubicSpline;
  }
  return NullOpt;
}

/// Parses and returns a span of floating point time data for |sampler.input|,
/// or NullOpt if the GLTF referred to an invalid accessor.
Optional<Span<float>> GetTimeData(const tinygltf::AnimationSampler& sampler,
                                  const tinygltf::Model& model) {
  // Fetch and verify the input accessor.
  const tinygltf::Accessor& frame_time_accessor =
      model.accessors[sampler.input];
  if (frame_time_accessor.bufferView == kInvalidTinyGltfIndex) {
    return NullOpt;
  }

  // According to the spec, times must be floating point values.
  if (frame_time_accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
    return NullOpt;
  }

  // Since the data has been verified, we can safely interpret it as a list of
  // floating point data.
  const auto* times = DataFromGltfAccessor<float>(model, frame_time_accessor);
  if (times == nullptr) {
    return NullOpt;
  }
  return Span<float>(times, frame_time_accessor.count);
}

/// Adds three spline-backed matrix operation animations to |anim|. The type
/// |base_type + i| will have use:
/// - |splines[i]| to store it's backing spline.
/// - The i'th component of the keyframes in |sampler|, which must reference an
///   Accessor of floating-point 3D vectors in |model|.
///
/// Returns true if the curves were successfully added, false otherwise.
bool AddVectorChannel(motive::MatrixAnim* anim,
                      motive::MatrixAnim::Spline* splines,
                      motive::MatrixOperationType base_type,
                      const tinygltf::AnimationSampler& sampler,
                      const tinygltf::Model& model) {
  Optional<Span<float>> time_data = GetTimeData(sampler, model);
  if (!time_data) {
    return false;
  }
  const tinygltf::Accessor& frame_values_accessor =
      model.accessors[sampler.output];

  // According to the spec, vector animations must be floating point values.
  if (frame_values_accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
    return false;
  }

  // Vector channels must be 3-dimensional vectors.
  if (frame_values_accessor.type != TINYGLTF_TYPE_VEC3) {
    return false;
  }

  auto interpolation_type = InterpolationTypeForString(sampler.interpolation);
  if (!interpolation_type) {
    return false;
  }

  // The input and output samplers need an equal number of keyframes. Since the
  // output sampler must be Vec3s, it's accessor should contain one value per
  // time. For CubicSpline interpolation, left and right tangents are included
  // in the output accessor, meaning it should have three times as many values.
  const size_t expected_num_output_values =
      *interpolation_type == motive::kCubicSpline ? time_data->size() * 3
                                                  : time_data->size();
  if (frame_values_accessor.count != expected_num_output_values) {
    return false;
  }

  // Since the data has been verified, we can safely interpret it as a list of
  // floating point data and use Motive to process it.
  const auto* values =
      DataFromGltfAccessor<float>(model, frame_values_accessor);
  if (values == nullptr) {
    return false;
  }

  motive::KeyframeData data;
  data.times = time_data->data();
  data.values = values;
  data.count = time_data->size();
  data.interpolation_type = *interpolation_type;
  motive::AddVector3Curves(anim, splines, base_type,
                           MatrixOpBaseIdFromBaseType(base_type), data);
  return true;
}

bool AddQuaternionChannel(motive::MatrixAnim* anim,
                          motive::MatrixAnim::Spline* splines,
                          const tinygltf::AnimationSampler& sampler,
                          const tinygltf::Model& model) {
  Optional<Span<float>> time_data = GetTimeData(sampler, model);
  if (!time_data) {
    return false;
  }
  const tinygltf::Accessor& frame_values_accessor =
      model.accessors[sampler.output];

  // TODO: handle other quaternion animation types in the spec.
  // Only handle float quaternion animations for now.
  if (frame_values_accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
    return false;
  }

  // Quaternion channels must be 4-dimensional vectors.
  if (frame_values_accessor.type != TINYGLTF_TYPE_VEC4) {
    return false;
  }

  auto interpolation_type = InterpolationTypeForString(sampler.interpolation);
  if (!interpolation_type) {
    return false;
  }

  // The input and output samplers need an equal number of keyframes. Since the
  // output sampler must be Vec3s, it's accessor should contain one value per
  // time. For CubicSpline interpolation, left and right tangents are included
  // in the output accessor, meaning it should have three times as many values.
  const size_t expected_num_output_values =
      *interpolation_type == motive::kCubicSpline ? time_data->size() * 3
                                                  : time_data->size();
  if (frame_values_accessor.count != expected_num_output_values) {
    return false;
  }

  // Since the data has been verified, we can safely interpret it as a list of
  // floating point data and use Motive to process it.
  const auto* values =
      DataFromGltfAccessor<float>(model, frame_values_accessor);
  if (values == nullptr) {
    return false;
  }

  motive::KeyframeData data;
  data.times = time_data->data();
  data.values = values;
  data.count = time_data->size();
  data.interpolation_type = *interpolation_type;
  motive::AddQuaternionCurves(anim, splines,
                              MatrixOpBaseIdFromBaseType(motive::kQuaternionW),
                              motive::kOrderXYZW, data);
  return true;
}

Optional<size_t> GetRequiredBufferSize(
    const tinygltf::AnimationSampler& sampler, const tinygltf::Model& model) {
  const Optional<motive::InterpolationType> type =
      InterpolationTypeForString(sampler.interpolation);
  if (!type) {
    return NullOpt;
  }
  const Optional<size_t> channel_count = GetChannelCount(sampler, model);
  if (!channel_count) {
    return NullOpt;
  }
  return motive::GetRequiredBufferSize(model.accessors[sampler.input].count,
                                       *channel_count, *type);
}

Optional<size_t> AddAnimationData(uint8_t* buffer,
                                  const tinygltf::AnimationSampler& sampler,
                                  const tinygltf::Model& model) {
  Optional<Span<float>> time_data = GetTimeData(sampler, model);
  if (!time_data) {
    return NullOpt;
  }
  const tinygltf::Accessor& frame_values_accessor =
      model.accessors[sampler.output];

  // TODO: support more than floating point animations.
  if (frame_values_accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
    return NullOpt;
  }

  Optional<size_t> channel_count = GetChannelCount(sampler, model);
  if (!channel_count) {
    return NullOpt;
  }

  auto interpolation_type = InterpolationTypeForString(sampler.interpolation);
  if (!interpolation_type) {
    return NullOpt;
  }

  // The input and output samplers need an equal number of keyframes. Since the
  // output sampler must be Vec3s, it's accessor should contain one value per
  // time. For CubicSpline interpolation, left and right tangents are included
  // in the output accessor, meaning it should have three times as many values.
  const size_t expected_num_output_values =
      *interpolation_type == motive::kCubicSpline ? time_data->size() * 3
                                                  : time_data->size();

  // Non-scalar accessors correctly report the number of values. Scalar ones
  // (e.g. blend weights) must be divided by the channel count to be accurate.
  const size_t num_values = frame_values_accessor.type == TINYGLTF_TYPE_SCALAR
                                ? frame_values_accessor.count / *channel_count
                                : frame_values_accessor.count;
  if (num_values != expected_num_output_values) {
    return NullOpt;
  }

  // Since the data has been verified, we can safely interpret it as a list of
  // floating point data and use Motive to process it.
  const auto* values =
      DataFromGltfAccessor<float>(model, frame_values_accessor);
  if (values == nullptr) {
    return NullOpt;
  }

  motive::KeyframeData data;
  data.times = time_data->data();
  data.values = values;
  data.count = time_data->size();
  data.interpolation_type = *interpolation_type;

  // Vec4 indicates quaternion curves, otherwise use array curves.
  if (frame_values_accessor.type == TINYGLTF_TYPE_VEC4) {
    return motive::AddQuaternionCurves(buffer, data, motive::kOrderXYZW);
  } else {
    return motive::AddArrayCurves(buffer, data, *channel_count);
  }
}

}  // namespace

size_t ElementSizeInBytes(const tinygltf::Accessor& accessor) {
  return tinygltf::GetComponentSizeInBytes(accessor.componentType) *
         tinygltf::GetTypeSizeInBytes(accessor.type);
}

size_t ByteStrideFromGltfAccessor(const tinygltf::Model& model,
                                  const tinygltf::Accessor& accessor) {
  if (accessor.bufferView == kInvalidTinyGltfIndex) {
    return 0;
  }
  const size_t stride = model.bufferViews[accessor.bufferView].byteStride;
  return stride != 0 ? stride : ElementSizeInBytes(accessor);
}

Optional<size_t> GetChannelCount(const tinygltf::AnimationSampler& sampler,
                                 const tinygltf::Model& model) {
  const tinygltf::Accessor& frame_values_accessor =
      model.accessors[sampler.output];
  if (frame_values_accessor.type == TINYGLTF_TYPE_VEC2) {
    return 2;
  } else if (frame_values_accessor.type == TINYGLTF_TYPE_VEC3) {
    return 3;
  } else if (frame_values_accessor.type == TINYGLTF_TYPE_VEC4) {
    return 4;
  }

  // If the accessor isn't for a type with a known channel count, assume it
  // accesses scalar arrays.
  if (frame_values_accessor.type != TINYGLTF_TYPE_SCALAR) {
    return NullOpt;
  }

  // The "input" is time values, meaning it counts the number of keyframes.
  // Dividing the total number of scalar values by the number of keyframes gives
  // the number of values per keyframe, which is the channel count.
  const tinygltf::Accessor& frame_time_accessor =
      model.accessors[sampler.input];
  size_t num_values = frame_values_accessor.count;

  // Cubicspline interpolation includes left and right tangents, which triples
  // the size of each keyframe.
  auto interpolation_type = InterpolationTypeForString(sampler.interpolation);
  if (!interpolation_type) {
    return NullOpt;
  } else if (*interpolation_type == motive::kCubicSpline) {
    if (num_values % 3 != 0) {
      return NullOpt;
    } else {
      num_values /= 3;
    }
  }

  if (num_values % frame_time_accessor.count != 0) {
    return NullOpt;
  }

  return num_values / frame_time_accessor.count;
}

Optional<size_t> GetRequiredBufferSize(const TinyGltfNodeAnimationData& data) {
  size_t bytes_required = 0;
  if (data.HasTranslation()) {
    Optional<size_t> size =
        GetRequiredBufferSize(*data.translation, data.model);
    if (!size) {
      return NullOpt;
    }
    bytes_required += *size;
  }
  if (data.HasRotation()) {
    Optional<size_t> size = GetRequiredBufferSize(*data.rotation, data.model);
    if (!size) {
      return NullOpt;
    }
    bytes_required += *size;
  }
  if (data.HasScale()) {
    Optional<size_t> size = GetRequiredBufferSize(*data.scale, data.model);
    if (!size) {
      return NullOpt;
    }
    bytes_required += *size;
  }
  if (data.HasWeights()) {
    Optional<size_t> size = GetRequiredBufferSize(*data.weights, data.model);
    if (!size) {
      return NullOpt;
    }
    bytes_required += *size;
  }
  return bytes_required;
}

Optional<size_t> AddAnimationData(uint8_t* buffer,
                                  const TinyGltfNodeAnimationData& data) {
  size_t bytes_used = 0;
  uint8_t* iter = buffer;
  if (data.HasTranslation()) {
    Optional<size_t> result =
        AddAnimationData(iter, *data.translation, data.model);
    if (!result) {
      return NullOpt;
    }
    bytes_used += *result;
    iter += *result;
  }
  if (data.HasRotation()) {
    Optional<size_t> result =
        AddAnimationData(iter, *data.rotation, data.model);
    if (!result) {
      return NullOpt;
    }
    bytes_used += *result;
    iter += *result;
  }
  if (data.HasScale()) {
    Optional<size_t> result = AddAnimationData(iter, *data.scale, data.model);
    if (!result) {
      return NullOpt;
    }
    bytes_used += *result;
    iter += *result;
  }
  if (data.HasWeights()) {
    Optional<size_t> result = AddAnimationData(iter, *data.weights, data.model);
    if (!result) {
      return NullOpt;
    }
    bytes_used += *result;
    iter += *result;
  }
  return bytes_used;
}

bool AddAnimationData(motive::MatrixAnim* matrix_anim,
                      const TinyGltfNodeAnimationData& data) {
  // TODO: consider adding animation compression.
  // Count the total number of splines required and construct a
  // MatrixAnim::Spline for each spline required. This pointer will advance
  // after each non-constant channel is added to the animation.
  int spline_count = 0;
  if (data.translation != nullptr) {
    spline_count += 3;
  }
  if (data.rotation != nullptr) {
    spline_count += 4;
  }
  if (data.scale != nullptr) {
    spline_count += 3;
  }
  motive::MatrixAnim::Spline* splines = matrix_anim->Construct(spline_count);

  if (data.translation != nullptr) {
    if (!AddVectorChannel(matrix_anim, splines, motive::kTranslateX,
                          *data.translation, data.model)) {
      return false;
    }
    splines += 3;
  } else {
    float translation[3] = {0, 0, 0};
    if (!data.node.translation.empty()) {
      translation[0] = static_cast<float>(data.node.translation[0]);
      translation[1] = static_cast<float>(data.node.translation[1]);
      translation[2] = static_cast<float>(data.node.translation[2]);
    }
    motive::AddVector3Constants(matrix_anim, motive::kTranslateX,
                                MatrixOpBaseIdFromBaseType(motive::kTranslateX),
                                translation);
  }

  if (data.rotation != nullptr) {
    if (!AddQuaternionChannel(matrix_anim, splines, *data.rotation,
                              data.model)) {
      return false;
    }
    splines += 4;
  } else {
    float rotation[4] = {0, 0, 0, 1};
    if (!data.node.rotation.empty()) {
      rotation[0] = static_cast<float>(data.node.rotation[0]);
      rotation[1] = static_cast<float>(data.node.rotation[1]);
      rotation[2] = static_cast<float>(data.node.rotation[2]);
      rotation[3] = static_cast<float>(data.node.rotation[3]);
    }
    motive::AddQuaternionConstants(
        matrix_anim, MatrixOpBaseIdFromBaseType(motive::kQuaternionW), rotation,
        motive::kOrderXYZW);
  }

  if (data.scale != nullptr) {
    if (!AddVectorChannel(matrix_anim, splines, motive::kScaleX, *data.scale,
                          data.model)) {
      return false;
    }
    splines += 3;
  } else {
    float scale[3] = {1, 1, 1};
    if (!data.node.scale.empty()) {
      scale[0] = static_cast<float>(data.node.scale[0]);
      scale[1] = static_cast<float>(data.node.scale[1]);
      scale[2] = static_cast<float>(data.node.scale[2]);
    }
    motive::AddVector3Constants(matrix_anim, motive::kScaleX,
                                MatrixOpBaseIdFromBaseType(motive::kScaleX),
                                scale);
  }
  return true;
}

}  // namespace lull
