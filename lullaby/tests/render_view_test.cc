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

#include <vector>

#include "gtest/gtest.h"
#include "lullaby/modules/render/render_view.h"
#include "lullaby/tests/mathfu_matchers.h"
#include "mathfu/glsl_mappings.h"

namespace lull {
namespace {

using testing::EqualsMathfu;

TEST(RenderView, GenerateEyeCenteredViews) {
  RenderView view;
  RenderView eye_centered_view;

  // Initialize the test view.
  view.viewport = mathfu::vec2i(1, 2);
  view.dimensions = mathfu::vec2i(3, 4);
  view.world_from_eye_matrix =
      mathfu::mat4::FromTranslationVector(mathfu::vec3(5, 6, 7));
  view.clip_from_eye_matrix = mathfu::mat4::Identity();
  view.clip_from_world_matrix =
      view.clip_from_eye_matrix * view.world_from_eye_matrix.Inverse();

  GenerateEyeCenteredViews({&view, 1}, &eye_centered_view);

  // Check the unchanging parts of eye centered view's contents are the same.
  EXPECT_THAT(view.viewport, EqualsMathfu(eye_centered_view.viewport));
  EXPECT_THAT(view.dimensions, EqualsMathfu(eye_centered_view.dimensions));
  EXPECT_THAT(view.clip_from_eye_matrix,
              EqualsMathfu(eye_centered_view.clip_from_eye_matrix));

  // Check that the world_from_eye_matrix's translation has been zeroed.
  EXPECT_THAT(eye_centered_view.world_from_eye_matrix.TranslationVector3D(),
              EqualsMathfu(mathfu::vec3(0, 0, 0)));

  // Check that the world_from_eye_matrix's rotation has been preserved.
  const mathfu::mat3& original_rotation =
      mathfu::mat4::ToRotationMatrix(view.world_from_eye_matrix);
  const mathfu::mat3& eye_centered_rotation =
      mathfu::mat4::ToRotationMatrix(eye_centered_view.world_from_eye_matrix);
  EXPECT_THAT(original_rotation, EqualsMathfu(eye_centered_rotation));
}

}  // namespace
}  // namespace lull
