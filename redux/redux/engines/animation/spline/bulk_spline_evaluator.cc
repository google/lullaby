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

#include "redux/engines/animation/spline/bulk_spline_evaluator.h"

namespace redux {

// These functions are implemented in assembly language.
extern "C" void UpdateCubicXsAndGetMask_Neon(const float& delta_x,
                                             const float* x_ends, int num_xs,
                                             float* xs, uint8_t* masks);

// y_range pointer is of type BulkSplineEvaluator::YRange (not used here because
// it's private, and extern "C" functions cannot be friends).
extern "C" void EvaluateCubics_Neon(const CubicCurve* curves, const float* xs,
                                    const void* y_ranges, int num_curves,
                                    float* ys);

inline float NormalizeInterval(const Interval& range, float x) {
  const float length = range.Size();
  const float adjustment = x <= range.min  ? length
                           : x > range.max ? -length
                                           : 0.f;
  return x + adjustment;
}

void BulkSplineEvaluator::SetNumIndices(const Index num_indices) {
  sources_.resize(num_indices);
  y_ranges_.resize(num_indices);
  cubic_xs_.resize(num_indices, 0.0f);
  cubic_x_ends_.resize(num_indices, 0.0f);
  cubics_.resize(num_indices);
  ys_.resize(num_indices, 0.0f);
  scratch_.resize(num_indices, 0);
}

void BulkSplineEvaluator::MoveIndices(const Index old_index,
                                      const Index new_index,
                                      const Index count) {
  for (Index i = 0; i < count; ++i) {
    const Index old_i = old_index + i;
    const Index new_i = new_index + i;
    sources_[new_i] = sources_[old_i];
    y_ranges_[new_i] = y_ranges_[old_i];
    cubic_xs_[new_i] = cubic_xs_[old_i];
    cubic_x_ends_[new_i] = cubic_x_ends_[old_i];
    cubics_[new_i] = cubics_[old_i];
    ys_[new_i] = ys_[old_i];
  }
}

void BulkSplineEvaluator::SetYRanges(const Index index, const Index count,
                                     const Interval& modular_range) {
  for (int i = index; i < index + count; ++i) {
    YRange& r = y_ranges_[i];
    r.modular_range = modular_range;
  }
}

CubicInit BulkSplineEvaluator::CalculateBlendInit(
    const Index index, const CompactSpline& spline,
    const SplinePlayback& playback) const {
  // Calculate spline segment where the blend will end.
  const float blend_width = playback.blend_x * playback.playback_rate;
  float blend_end_x = 0.0f;
  const CompactSplineIndex blend_end_index = spline.IndexForXAllowingRepeat(
      playback.start_x + blend_width, kInvalidSplineIndex, playback.repeat,
      &blend_end_x);

  // Gather the spline values. Only create the cubic if we have to.
  float end_y = 0.0f;
  float end_derivative = 0.0f;
  if (OutsideSpline(blend_end_index)) {
    // Get the start or end y-values of the spline.
    end_y = spline.NodeY(blend_end_index);
  } else {
    // Create the cubic for the end segment.
    const float curve_x = blend_end_x - spline.NodeX(blend_end_index);
    const CubicInit curve_init = spline.CreateCubicInit(blend_end_index);
    const CubicCurve curve(curve_init);
    end_y = curve.Evaluate(curve_x);
    end_derivative = curve.Derivative(curve_x);
  }

  // Scale and shift the end value by the playback parameters.
  end_y *= playback.y_scale;
  end_y += playback.y_offset;

  // Use the current values for the curve start.
  float start_y = ys_[index];
  const float start_derivative = Derivative(index);

  // Account for modular arithmentic. Always start in the normalized range.
  const YRange& r = y_ranges_[index];
  if (r.modular_range.Size() > 0.f) {
    // We take the shortest modular path to the new curve.
    // So if we're blending from angle 170 to angle -170 (=+190),
    // we will blend from 170-->190 instead of 170-->-170.
    start_y = NormalizeCloseValueWithinInterval(r.modular_range, start_y);
    const float end_y_normalized =
        NormalizeCloseValueWithinInterval(r.modular_range, end_y);
    const float diff_y =
        NormalizeInterval(r.modular_range, end_y_normalized - start_y);
    end_y = start_y + diff_y;
  }

  // Return the cubic parameters.
  return CubicInit(start_y, start_derivative, end_y, end_derivative,
                   blend_width);
}

void BulkSplineEvaluator::BlendToSpline(const Index index,
                                        const CompactSpline& spline,
                                        const SplinePlayback& playback) {
  // Calculate the spline that transitions from the current curve state
  // to the target spline's state.
  // Transition spline runs from x=0-->playback.blend_time.
  const CubicInit blend_init = CalculateBlendInit(index, spline, playback);

  // Shift the transition spline so that it overlaps perfectly onto the target
  // spline. Initialize all the x-parameters as if we were initializing the
  // target spline. This will let us transition out of the transition spline
  // straight into the target spline without special casing.
  float blend_start_x = 0.0f;
  const CompactSplineIndex blend_start_index = spline.IndexForXAllowingRepeat(
      playback.start_x, kInvalidSplineIndex, playback.repeat, &blend_start_x);
  const float cubic_start_x = blend_start_x - spline.NodeX(blend_start_index);

  Source& s = sources_[index];
  s.rate = playback.playback_rate;
  s.y_offset = playback.y_offset;
  s.y_scale = playback.y_scale;
  s.spline = &spline;
  s.x_index = blend_start_index;
  s.repeat = playback.repeat;
  cubic_xs_[index] = cubic_start_x;
  cubic_x_ends_[index] =
      cubic_start_x + playback.blend_x * playback.playback_rate;
  cubics_[index].Init(blend_init);
  cubics_[index].ShiftRight(cubic_start_x);
}

void BulkSplineEvaluator::JumpToSpline(const Index index,
                                       const CompactSpline& spline,
                                       const SplinePlayback& playback) {
  Source& s = sources_[index];
  s.rate = playback.playback_rate;
  s.y_offset = playback.y_offset;
  s.y_scale = playback.y_scale;
  s.spline = &spline;
  s.x_index = kInvalidSplineIndex;
  s.repeat = playback.repeat;
  InitCubic(index, playback.start_x);
}

void BulkSplineEvaluator::SetSplines(const Index index, const Index count,
                                     const CompactSpline* splines,
                                     const SplinePlayback& playback) {
  const CompactSpline* spline = splines;
  for (Index i = index; i < index + count; ++i, spline = spline->Next()) {
    // `splines` should specify `count` splines, but gracefully handle the
    // case when it doesn't.
    if (spline == nullptr) {
      ClearSplines(i, count - i + index);
      break;
    }

    // If we're already playing a spline, and the blend time is specified,
    // create a curve that blends from the current state to a point later in
    // the new spline.
    const Source& s = sources_[i];
    const bool should_blend = s.spline != nullptr && playback.blend_x > 0.0f;
    if (should_blend) {
      BlendToSpline(i, *spline, playback);
    } else {
      JumpToSpline(i, *spline, playback);
    }

    // Update the results.
    // TODO OPT: Evaluate these in bulk.
    EvaluateIndex(i);
  }
}

void BulkSplineEvaluator::Splines(const Index index, const Index count,
                                  const CompactSpline** splines) const {
  for (Index i = 0; i < count; ++i) {
    splines[i] = sources_[index + i].spline;
  }
}

void BulkSplineEvaluator::ClearSplines(const Index index, const Index count) {
  for (Index i = index; i < index + count; ++i) {
    sources_[i].spline = nullptr;
    cubics_[i] = CubicCurve(0.0f, 0.0f, 0.0f, cubic_xs_[i]);
    cubic_xs_[i] = 0.0f;
    cubic_x_ends_[i] = std::numeric_limits<float>::infinity();
  }
}

void BulkSplineEvaluator::SetXs(const Index index, const Index count,
                                const float x) {
  for (Index i = index; i < index + count; ++i) {
    InitCubic(i, x);
    EvaluateIndex(i);
  }
}

void BulkSplineEvaluator::SetPlaybackRates(const Index index, const Index count,
                                           float playback_rate) {
  for (Index i = index; i < index + count; ++i) {
    sources_[i].rate = playback_rate;
  }
}

void BulkSplineEvaluator::SetRepeating(const Index index, const Index count,
                                       bool repeat) {
  for (Index i = index; i < index + count; ++i) {
    sources_[i].repeat = repeat;
  }
}

void BulkSplineEvaluator::UpdateCubicXsAndGetMask_C(const float delta_x,
                                                    uint8_t* masks) {
  const int num_xs = NumIndices();
  const float* x_ends = &cubic_x_ends_.front();
  float* xs = &cubic_xs_.front();

  for (int i = 0; i < num_xs; ++i) {
    xs[i] += delta_x * sources_[i].rate;
    masks[i] = xs[i] > x_ends[i] ? 0xFF : 0x00;
  }
}

// For each non-zero mask[i], append 'i' to 'indices'.
// Returns: final length of indices.
// TODO OPT: Add assembly version if generated code is poor.
static size_t ConvertMaskToIndices(const uint8_t* mask, size_t length,
                                   BulkSplineEvaluator::Index* indices) {
  size_t num_indices = 0;
  for (size_t i = 0; i < length; ++i) {
    indices[num_indices] = static_cast<uint16_t>(i);
    if (mask[i] != 0) {
      num_indices++;
    }
  }
  return num_indices;
}

// Get a byte mask for the indices to init, and then convert that byte mask
// into a list of indices. This algorithm is best for many SIMD implementations,
// since they have trouble converting masks into indices.
size_t BulkSplineEvaluator::UpdateCubicXs_TwoSteps(const float delta_x,
                                                   Index* indices_to_init) {
  // Use last half of 'indices_to_init' as a scratch buffer for 'mask'.
  // Must be the last half since we read 'mask' to write 'indices_to_init'
  // in ConvertMaskToIndices().
  const Index num_indices = NumIndices();
  uint8_t* mask = reinterpret_cast<uint8_t*>(&indices_to_init[num_indices / 2]);

  // Add delta_x to each of the cubic_xs_.
  // Set mask[i] to 0xFF if the cubic has gone past the end of its array.
  UpdateCubicXsAndGetMask(delta_x, mask);

  // Get indices that are true 0xFF in the mask array.
  return ConvertMaskToIndices(mask, num_indices, indices_to_init);
}

// Record the indices, as we go along, for every index we need to re-init.
// This algorithm is fastest when we process indices serially.
size_t BulkSplineEvaluator::UpdateCubicXs_OneStep(const float delta_x,
                                                  Index* indices_to_init) {
  const Index num_indices = NumIndices();
  size_t num_to_init = 0;

  for (Index i = 0; i < num_indices; ++i) {
    // Increment each cubic x value by delta_x.
    cubic_xs_[i] += delta_x * sources_[i].rate;

    // When x has gone past the end of the cubic, it should be reinitialized.
    if (cubic_xs_[i] > cubic_x_ends_[i]) {
      indices_to_init[num_to_init++] = i;
    }
  }
  return num_to_init;
}

void BulkSplineEvaluator::InitCubic(const Index index, const float start_x) {
  // Do nothing if the requested index has no spline.
  Source& s = sources_[index];
  if (s.spline == nullptr) return;

  // Get the spline index for start_x.
  float new_start_x = 0.0f;
  const CompactSplineIndex x_index = s.spline->IndexForXAllowingRepeat(
      start_x, s.x_index + 1, s.repeat, &new_start_x);

  // Update the x values for the new index.
  const Interval x_range = s.spline->IntervalX(x_index);
  cubic_xs_[index] = new_start_x - x_range.min;

  // TODO OPT: Exit early if s.x_index == x_index, since we've already
  //   initialized the cubic. This is tricky, since if we're blending then the
  //   index might match, but the cubic curve will not mach. We should refactor
  //   to detect that case, so we can skip over the CreateCubicInit() call.
  s.x_index = x_index;

  // Initialize the cubic to interpolate the new spline segment.
  cubic_x_ends_[index] = x_range.Size();
  CubicCurve& c = cubics_[index];
  const CubicInit init = s.spline->CreateCubicInit(x_index);
  c.Init(init);

  c.ScaleUp(s.y_scale);
  c.ShiftUp(s.y_offset);
}

void BulkSplineEvaluator::EvaluateIndex(const Index index) {
  // Evaluate the cubic spline.
  CubicCurve& c = cubics_[index];
  ys_[index] = c.Evaluate(cubic_xs_[index]);
}

void BulkSplineEvaluator::EvaluateCubics_C() {
  for (Index index = 0; index < NumIndices(); ++index) {
    EvaluateIndex(index);
  }
}

void BulkSplineEvaluator::AdvanceFrame(const float delta_x) {
  // Add 'delta_x' to 'cubic_xs'.
  // Gather a list of indices that are now beyond the end of the cubic.
  Index* indices_to_init = scratch_.size() == 0 ? nullptr : &scratch_.front();
  const size_t num_to_init = UpdateCubicXs(delta_x, indices_to_init);

  // Reinitialize indices that have traversed beyond the end of their cubic.
  for (size_t i = 0; i < num_to_init; ++i) {
    const Index index = indices_to_init[i];
    InitCubic(index, X(index));
  }

  // Update 'ys_' array. Also might affect the constant coefficients of
  // 'cubics_', if we're adjusting for modular arithmetic.
  EvaluateCubics();
}

bool BulkSplineEvaluator::Valid(const Index index) const {
  return 0 <= index && index < NumIndices() &&
         sources_[index].spline != nullptr;
}

// Form the assembly function name by appending "_Neon", "_SSD2", or whatever
// REDUX_ANIM_ASSEMBLY_TEST is defined to be.
#define REDUX_ANIM_TOKEN_PASTE_NESTED(a, b) a##b
#define REDUX_ANIM_TOKEN_PASTE(a, b) REDUX_ANIM_TOKEN_PASTE_NESTED(a, b)
#define REDUX_ANIM_ASSEMBLY_FUNCTION_NAME(name) \
  REDUX_ANIM_TOKEN_PASTE(name, REDUX_ANIM_ASSEMBLY_TEST)

// These inline functions are used to redirect calls to the C or assembly
// versions, or to run both versions and compare the output.
inline void BulkSplineEvaluator::UpdateCubicXsAndGetMask(const float delta_x,
                                                         uint8_t* masks) {
#if defined(REDUX_ANIM_ASSEMBLY_TEST)
  const int num_xs = NumIndices();
  std::vector<float> xs_assembly(cubic_xs_);
  std::vector<uint8_t> masks_assembly(num_xs);

  UpdateCubicXsAndGetMask_C(delta_x, masks);
  REDUX_ANIM_ASSEMBLY_FUNCTION_NAME(UpdateCubicXsAndGetMask_)
  (delta_x, &cubic_x_ends_.front(), num_xs, &xs_assembly.front(),
   &masks_assembly.front());

  for (int i = 0; i < num_xs; ++i) {
    assert(cubic_xs_[i] == xs_assembly[i]);
    assert(masks[i] == masks_assembly[i]);
  }

#else  // not defined(REDUX_ANIM_ASSEMBLY_TEST)

#if defined(REDUX_ANIM_NEON)
  if (optimization_ == kNeonOptimizations) {
    UpdateCubicXsAndGetMask_Neon(delta_x, &cubic_x_ends_.front(), NumIndices(),
                                 &cubic_xs_.front(), masks);
  } else
#endif
  {
    UpdateCubicXsAndGetMask_C(delta_x, masks);
  }

#endif  // not defined(REDUX_ANIM_ASSEMBLY_TEST)
}

inline size_t BulkSplineEvaluator::UpdateCubicXs(const float delta_x,
                                                 Index* indices_to_init) {
#if defined(REDUX_ANIM_ASSEMBLY_TEST)
  std::vector<float> xs_original(cubic_xs_);
  std::vector<Index> indices_one(NumIndices());

  const size_t num_one = UpdateCubicXs_OneStep(delta_x, &indices_one.front());
  std::vector<float> xs_one(cubic_xs_);

  cubic_xs_ = xs_original;
  const size_t num_two = UpdateCubicXs_TwoSteps(delta_x, indices_to_init);

  assert(num_two == num_one);
  for (size_t i = 0; i < num_two; ++i) {
    assert(indices_to_init[i] == indices_one[i]);
  }
  for (int i = 0; i < NumIndices(); ++i) {
    assert(cubic_xs_[i] == xs_one[i]);
  }
  return num_two;

#else  // not defined(REDUX_ANIM_ASSEMBLY_TEST)

#if defined(REDUX_ANIM_NEON)
  return UpdateCubicXs_TwoSteps(delta_x, indices_to_init);
#else
  return UpdateCubicXs_OneStep(delta_x, indices_to_init);
#endif

#endif  // not defined(REDUX_ANIM_ASSEMBLY_TEST)
}

inline void BulkSplineEvaluator::EvaluateCubics() {
#if defined(REDUX_ANIM_ASSEMBLY_TEST)
  std::vector<float> ys_assembly(NumIndices());
  std::vector<CubicCurve> cubics_assembly(cubics_);

  REDUX_ANIM_ASSEMBLY_FUNCTION_NAME(EvaluateCubics_)
  (&cubics_assembly.front(), &cubic_xs_.front(), &y_ranges_.front(),
   NumIndices(), &ys_assembly.front());
  EvaluateCubics_C();

  for (int i = 0; i < NumIndices(); ++i) {
    assert(ys_assembly[i] == ys_[i]);
  }
  for (int i = 0; i < NumIndices(); ++i) {
    assert(cubics_assembly[i] == cubics_[i]);
  }
#else  // not defined(REDUX_ANIM_ASSEMBLY_TEST)

#if defined(REDUX_ANIM_NEON)
  EvaluateCubics_Neon(&cubics_.front(), &cubic_xs_.front(), &y_ranges_.front(),
                      NumIndices(), &ys_.front());
#else
  EvaluateCubics_C();
#endif

#endif  // not defined(REDUX_ANIM_ASSEMBLY_TEST)
}

}  // namespace redux
