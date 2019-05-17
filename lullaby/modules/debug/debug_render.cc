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

#include "lullaby/modules/debug/debug_render.h"

#include <mutex>

#include "lullaby/modules/debug/debug_render_draw_interface.h"
#include "lullaby/modules/debug/log_tag.h"

namespace lull {
namespace debug {

class DebugRender {
 public:
  explicit DebugRender(DebugRenderDrawInterface* interface);

  ~DebugRender();

  // Adds line between two given points to the element buffer.
  void AddLine(const string_view tag, const mathfu::vec3& start_point,
               const mathfu::vec3& end_point, Color4ub color);

  // Adds line connecting given points in sequence to the element buffer.
  void AddLineStrip(const string_view tag,
                    const std::vector<mathfu::vec3>& points, Color4ub color);

  // Adds billboard text and its position to the element buffer.
  void AddText3D(const string_view tag, const mathfu::vec3& pos, Color4ub color,
                 const char* text);

  // Adds screen space text to the element buffer.
  void AddText2D(const string_view tag, Color4ub color, const char* text);

  // Adds a 3D box to the debug render queue.
  void AddBox3D(const string_view tag,
                const mathfu::mat4& world_from_object_matrix, const Aabb& box,
                Color4ub color);

  // Adds a 2D screen space quad to the debug render queue.
  void AddQuad2D(const string_view tag, Color4ub color, float x, float y,
                 float w, float h, const TexturePtr& texture);

  // Adds a 2D screen-space quad to the debug render queue, using pixel units.
  // * Origin is at the top-left and position is in pixel units.
  // * UVs default to the [0, 1] range.
  void AddQuad2DAbsolute(
      string_view tag, const mathfu::vec4& color,
      const mathfu::vec2& pixel_pos0, const mathfu::vec2& uv0,
      const mathfu::vec2& pixel_pos1, const mathfu::vec2& uv1,
      const TexturePtr& texture);

  // Swaps write and read buffers.
  void Swap();

  // Commits all drawing data and clears read buffer.
  void Submit();

 private:
  static constexpr const int kMaxTextLength = 256;

  // TODO Sort Debug elements based on their position + type.
  // Currently the order is based solely on type.
  enum class Type {
    kBox3D,
    kText3D,
    kLine,
    kText2D,
    kQuad2D,
    kQuad2DAbsolute,
  };

  // TODO: Use a union or type-specific structures to save space.
  struct DrawElement {
    string_view tag;
    mathfu::mat4 world_from_object_matrix;
    mathfu::vec3 pos0;
    mathfu::vec3 pos1;
    mathfu::vec2 uv0;
    mathfu::vec2 uv1;
    Aabb box;
    mathfu::vec4 color;
    TexturePtr texture;
    char text[kMaxTextLength];
    Type type;
  };

  using Lock = std::unique_lock<std::mutex>;

  DebugRenderDrawInterface* draw_;
  std::vector<DrawElement> buffers_[2];
  int read_index_ = 0;
  std::mutex mutex_;

  friend bool operator<(const DrawElement& lhs, const DrawElement& rhs);

  DebugRender(const DebugRender&) = delete;
  DebugRender& operator=(const DebugRender&) = delete;
};

bool operator<(const DebugRender::DrawElement& lhs,
               const DebugRender::DrawElement& rhs) {
  return lhs.type < rhs.type;
}

std::unique_ptr<DebugRender> gDebugRender;

DebugRender::DebugRender(DebugRenderDrawInterface* interface)
    : draw_(interface) {}

DebugRender::~DebugRender() {}

void DebugRender::AddLine(const string_view tag,
                          const mathfu::vec3& start_point,
                          const mathfu::vec3& end_point, const Color4ub color) {
  IsEnabled(tag);
  DebugRender::DrawElement element;
  element.tag = tag;
  element.pos0 = start_point;
  element.pos1 = end_point;
  element.color = Color4ub::ToVec4(color);
  element.type = DebugRender::Type::kLine;

  Lock lock(mutex_);
  const int write_index = 1 - read_index_;
  buffers_[write_index].emplace_back(element);
}

void DebugRender::AddLineStrip(const string_view tag,
                               const std::vector<mathfu::vec3>& points,
                               const Color4ub color) {
  if (points.size() < 2) {
    LOG(DFATAL) << "Line strip must have at least 2 points!";
    return;
  }
  for (size_t i = 0; i < points.size() - 1; ++i) {
    DebugRender::AddLine(tag, points[i], points[i + 1], color);
  }
}

void DebugRender::AddText3D(const string_view tag, const mathfu::vec3& pos,
                            const Color4ub color, const char* text) {
  IsEnabled(tag);
  DebugRender::DrawElement element;
  element.tag = tag;
  element.pos0 = pos;
  element.color = Color4ub::ToVec4(color);
  strncpy(element.text, text, sizeof(element.text));
  element.type = DebugRender::Type::kText3D;

  Lock lock(mutex_);
  const int write_index = 1 - read_index_;
  buffers_[write_index].emplace_back(element);
}

void DebugRender::AddText2D(const string_view tag, const Color4ub color,
                            const char* text) {
  IsEnabled(tag);
  DebugRender::DrawElement element;
  element.tag = tag;
  element.color = Color4ub::ToVec4(color);
  strncpy(element.text, text, sizeof(element.text));
  element.type = DebugRender::Type::kText2D;

  Lock lock(mutex_);
  const int write_index = 1 - read_index_;
  buffers_[write_index].emplace_back(element);
}

void DebugRender::AddBox3D(const string_view tag,
                           const mathfu::mat4& world_from_object_matrix,
                           const Aabb& box, Color4ub color) {
  IsEnabled(tag);
  DebugRender::DrawElement element;
  element.tag = tag;
  element.world_from_object_matrix = world_from_object_matrix;
  element.box = box;
  element.color = Color4ub::ToVec4(color);
  element.type = DebugRender::Type::kBox3D;

  Lock lock(mutex_);
  const int write_index = 1 - read_index_;
  buffers_[write_index].emplace_back(element);
}

void DebugRender::AddQuad2D(const string_view tag, Color4ub color, float x,
                            float y, float w, float h,
                            const TexturePtr& texture) {
  IsEnabled(tag);
  DebugRender::DrawElement element;
  element.tag = tag;
  element.pos0 = mathfu::vec3(x, y, 0);
  element.pos1 = mathfu::vec3(w, h, 0);
  element.color = Color4ub::ToVec4(color);
  element.texture = texture;
  element.type = DebugRender::Type::kQuad2D;

  Lock lock(mutex_);
  const int write_index = 1 - read_index_;
  buffers_[write_index].emplace_back(element);
}

void DebugRender::AddQuad2DAbsolute(
    string_view tag, const mathfu::vec4& color,
    const mathfu::vec2& pixel_pos0, const mathfu::vec2& uv0,
    const mathfu::vec2& pixel_pos1, const mathfu::vec2& uv1,
    const TexturePtr& texture) {
  IsEnabled(tag);
  DebugRender::DrawElement element;
  element.tag = tag;
  element.pos0 = mathfu::vec3(pixel_pos0.x, pixel_pos0.y, 0.0f);
  element.pos1 = mathfu::vec3(pixel_pos1.x, pixel_pos1.y, 0.0f);
  element.uv0 = uv0;
  element.uv1 = uv1;
  element.color = color;
  element.texture = texture;
  element.type = DebugRender::Type::kQuad2DAbsolute;

  Lock lock(mutex_);
  const int write_index = 1 - read_index_;
  buffers_[write_index].emplace_back(element);
}

void DebugRender::Swap() {
  Lock lock(mutex_);
  read_index_ = 1 - read_index_;
}

void DebugRender::Submit() {
  Swap();
  std::vector<DrawElement>& curr_buffer = buffers_[read_index_];
  std::stable_sort(curr_buffer.begin(), curr_buffer.end());
  for (DrawElement& element : curr_buffer) {
    if (IsEnabled(element.tag)) {
      const mathfu::vec4& color = element.color;
      switch (element.type) {
        case DebugRender::Type::kLine:
          draw_->DrawLine(element.pos0, element.pos1,
                          lull::Color4ub::FromVec4(color));
          break;
        case DebugRender::Type::kText3D:
          draw_->DrawText3D(element.pos0, lull::Color4ub::FromVec4(color),
                            element.text);
          break;
        case DebugRender::Type::kBox3D:
          draw_->DrawBox3D(element.world_from_object_matrix, element.box,
                           lull::Color4ub::FromVec4(color));
          break;
        case DebugRender::Type::kText2D:
          draw_->DrawText2D(lull::Color4ub::FromVec4(color), element.text);
          break;
        case DebugRender::Type::kQuad2D:
          draw_->DrawQuad2D(lull::Color4ub::FromVec4(color), element.pos0.x,
                            element.pos0.y, element.pos1.x, element.pos1.y,
                            element.texture);
          break;
        case DebugRender::Type::kQuad2DAbsolute:
          draw_->DrawQuad2DAbsolute(color,
                                    element.pos0.xy(), element.uv0,
                                    element.pos1.xy(), element.uv1,
                                    element.texture);
          break;
      }
    }
  }
  buffers_[read_index_].clear();
}

void Initialize(DebugRenderDrawInterface* interface) {
  gDebugRender.reset(new DebugRender(interface));
  InitializeLogTag();
}

bool IsInitialized() {
  return gDebugRender != nullptr;
}

void Shutdown() {
  gDebugRender.reset(nullptr);
  ShutdownLogTag();
}

void DrawLine(const char* tag_name, const mathfu::vec3& start_point,
              const mathfu::vec3& end_point, const Color4ub color) {
  if (gDebugRender) {
    gDebugRender->AddLine(tag_name, start_point, end_point, color);
  }
}

void DrawTransformAxes(const char* tag_name,
                       const mathfu::mat4& world_from_object_matrix) {
  const mathfu::vec3 basis_x = GetMatrixColumn3D(world_from_object_matrix, 0);
  const mathfu::vec3 basis_y = GetMatrixColumn3D(world_from_object_matrix, 1);
  const mathfu::vec3 basis_z = GetMatrixColumn3D(world_from_object_matrix, 2);
  const mathfu::vec3 position = GetMatrixColumn3D(world_from_object_matrix, 3);
  DrawLine(tag_name, position, position + basis_x, Color4ub(255, 0, 0, 255));
  DrawLine(tag_name, position, position + basis_y, Color4ub(0, 255, 0, 255));
  DrawLine(tag_name, position, position + basis_z, Color4ub(0, 0, 255, 255));
}

void DrawLineStrip(const char* tag_name,
                   const std::vector<mathfu::vec3>& points,
                   const Color4ub color) {
  if (gDebugRender) {
    gDebugRender->AddLineStrip(tag_name, points, color);
  }
}

void DrawText3D(const char* tag_name, const mathfu::vec3& pos,
                const Color4ub color, const char* text) {
  if (gDebugRender) {
    gDebugRender->AddText3D(tag_name, pos, color, text);
  }
}

void DrawText2D(const char* tag_name, const Color4ub color, const char* text) {
  if (gDebugRender) {
    gDebugRender->AddText2D(tag_name, color, text);
  }
}

void DrawBox3D(const char* tag_name,
               const mathfu::mat4& world_from_object_matrix, const Aabb& box,
               Color4ub color) {
  if (gDebugRender) {
    gDebugRender->AddBox3D(tag_name, world_from_object_matrix, box, color);
  }
}

void DrawQuad2D(const char* tag_name, Color4ub color, float x, float y, float w,
                float h, const TexturePtr& texture) {
  if (gDebugRender) {
    gDebugRender->AddQuad2D(tag_name, color, x, y, w, h, texture);
  }
}

void DrawQuad2DAbsolute(
    const char* tag_name, const mathfu::vec4& color,
    const mathfu::vec2& pixel_pos0, const mathfu::vec2& uv0,
    const mathfu::vec2& pixel_pos1, const mathfu::vec2& uv1,
    const TexturePtr& texture) {
  if (gDebugRender) {
    gDebugRender->AddQuad2DAbsolute(
        tag_name, color, pixel_pos0, uv0, pixel_pos1, uv1, texture);
  }
}

void DrawQuad2DAbsolute(const char* tag_name,
                      const mathfu::vec4& color,
                      const mathfu::vec2& pixel_pos0,
                      const mathfu::vec2& pixel_pos1,
                      const TexturePtr& texture) {
  DrawQuad2DAbsolute(tag_name, color,
                     pixel_pos0, mathfu::kZeros2f,
                     pixel_pos1, mathfu::kOnes2f,
                     texture);
}

void DrawQuad2DAbsolute(
    const char* tag_name, Color4ub color,
    const mathfu::vec2& pixel_pos0, const mathfu::vec2& uv0,
    const mathfu::vec2& pixel_pos1, const mathfu::vec2& uv1,
    const TexturePtr& texture) {
  DrawQuad2DAbsolute(tag_name, Color4ub::ToVec4(color),
                     pixel_pos0, uv0,
                     pixel_pos1, uv1,
                     texture);
}

void DrawQuad2DAbsolute(
    const char* tag_name, Color4ub color,
    const mathfu::vec2& pixel_pos0,
    const mathfu::vec2& pixel_pos1,
    const TexturePtr& texture) {
  DrawQuad2DAbsolute(tag_name, Color4ub::ToVec4(color),
                     pixel_pos0, mathfu::kZeros2f,
                     pixel_pos1, mathfu::kOnes2f,
                     texture);
}

void Submit() {
  if (gDebugRender) {
    gDebugRender->Submit();
  }
}

}  // namespace debug
}  // namespace lull
