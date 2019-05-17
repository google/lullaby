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

#include "lullaby/viewer/src/widgets/preview_window.h"

#include "dear_imgui/imgui.h"
#include "fplbase/glplatform.h"
#include "lullaby/systems/render/render_system.h"

namespace lull {
namespace tool {

PreviewWindow::PreviewWindow(Registry* registry, size_t width, size_t height)
    : registry_(registry), width_(width), height_(height) {
  glGenFramebuffers(1, &framebuffer_);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

  glGenRenderbuffers(1, &depthbuffer_);
  glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, depthbuffer_);

  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture_, 0);

  auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  CHECK(status == GL_FRAMEBUFFER_COMPLETE) << "Invalid framebuffer.";

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

PreviewWindow::~PreviewWindow() {
  if (framebuffer_) {
    glDeleteFramebuffers(1, &framebuffer_);
    framebuffer_ = 0;
  }
  if (depthbuffer_) {
    glDeleteRenderbuffers(1, &depthbuffer_);
    depthbuffer_ = 0;
  }
  if (texture_) {
    glDeleteTextures(1, &texture_);
    texture_ = 0;
  }
}

void PreviewWindow::AdvanceFrame() {
  const int flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoCollapse;
  if (ImGui::Begin("Preview Window", nullptr, flags)) {
    const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    const ImVec2 image_position(cursor_pos.x, cursor_pos.y);
    const ImVec2 image_size(cursor_pos.x + width_, cursor_pos.y + height_);

    CheckInput();


    Render();
    ImGui::GetWindowDrawList()->AddImage((void*)(intptr_t)texture_,
                                         image_position, image_size,
                                         ImVec2(0, 1), ImVec2(1, 0));
  }
  ImGui::End();
}

void PreviewWindow::CheckInput() {
  if (!ImGui::IsWindowFocused()) {
    return;
  }

  if (ImGui::IsMouseDragging()) {
    static constexpr float kTranslationSensitivity = 0.01f;
    static constexpr float kRotationSensitivity = 0.25f * kDegreesToRadians;

    const bool ctrl = ImGui::GetIO().KeyCtrl;
    const bool shift = ImGui::GetIO().KeyShift;
    const ImVec2 delta = ImGui::GetMouseDragDelta();
    ImGui::ResetMouseDragDelta();

    if (ctrl && shift) {
      // Roll rotation.
      rotation_.z += delta.y * kRotationSensitivity;
    } else if (shift) {
      // X and Z translation.
      const mathfu::vec3 delta_xz(static_cast<float>(delta.x), 0.0f,
                                  static_cast<float>(delta.y));
      translation_ -= delta_xz * kTranslationSensitivity;
    } else if (ctrl) {
      // X and Y translation.
      const mathfu::vec3 delta_xy(static_cast<float>(delta.x),
                                  -static_cast<float>(delta.y), 0.0f);
      translation_ -= delta_xy * kTranslationSensitivity;
    } else {
      // Pitch and yaw rotation.
      rotation_.y -= delta.x * kRotationSensitivity;
      rotation_.x -= delta.y * kRotationSensitivity;
    }
  }
}

void PreviewWindow::Render() {
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

  const float kNearClip = 0.1f;
  const float kFarClip = 1000.f;
  const float kFovAngle = 45.f * lull::kDegreesToRadians;
  const float aspect_ratio =
      static_cast<float>(width_) / static_cast<float>(height_);
  const mathfu::mat4 clip_from_eye_transform =
      CalculatePerspectiveMatrixFromView(kFovAngle, aspect_ratio, kNearClip,
                                         kFarClip);

  const mathfu::vec3& pos = translation_;
  const mathfu::quat& rot = lull::FromEulerAnglesYXZ(rotation_);
  RenderView view;
  view.eye = 0;
  const auto rot_matrix =
      CalculateTransformMatrix(mathfu::kZeros3f, rot, mathfu::kOnes3f);
  const auto pos_matrix =
      CalculateTransformMatrix(pos, mathfu::kQuatIdentityf, mathfu::kOnes3f);
  view.world_from_eye_matrix = rot_matrix * pos_matrix;
  view.eye_from_world_matrix = view.world_from_eye_matrix.Inverse();
  view.clip_from_eye_matrix = clip_from_eye_transform;
  view.clip_from_world_matrix =
      clip_from_eye_transform * view.eye_from_world_matrix;
  view.viewport = mathfu::kZeros2i;
  view.dimensions = mathfu::vec2i(width_, height_);

  auto* render_system = registry_->Get<lull::RenderSystem>();

  render_system->BeginFrame();
  render_system->BeginRendering();
  render_system->SetClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  render_system->Render(
      &view, 1, static_cast<lull::RenderPass>(lull::ConstHash("ClearDisplay")));
  render_system->Render(&view, 1);
  render_system->EndRendering();
  render_system->EndFrame();

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}  // namespace tool
}  // namespace lull
