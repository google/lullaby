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

#ifndef LULLABY_SYSTEMS_TEXT_FLATUI_FLATUI_TEXT_SYSTEM_H_
#define LULLABY_SYSTEMS_TEXT_FLATUI_FLATUI_TEXT_SYSTEM_H_

#include <memory>
#include <string>

#include "flatui/font_manager.h"
#include "fplbase/renderer.h"
#include "lullaby/events/layout_events.h"
#include "lullaby/systems/text/flatui/text_buffer.h"
#include "lullaby/systems/text/flatui/text_component.h"
#include "lullaby/systems/text/text_system.h"
#include "lullaby/util/async_processor.h"
#include "lullaby/generated/text_def_generated.h"

namespace lull {

// The flatui implementation of the TextSystem.  This mostly implements the
// TextSystem API; refer to text_system.h for public documentation.
class FlatuiTextSystem : public TextSystemImpl {
 public:
  explicit FlatuiTextSystem(Registry* registry);
  ~FlatuiTextSystem() override;

  // Sets the size and max depth of the font glyph cache, which is shared across
  // all active glyphs regardless of font. |size| is the pixel dimensions of
  // each slice, with |max_slices| defining the maximum number of these 2D
  // images.
  //
  // This function must be called before Initialize, which is typically invoked
  // via EntityFactory::Initialize.
  void SetGlyphCacheSize(const mathfu::vec2i& size, int max_slices);

  void Initialize() override;
  void Create(Entity entity, DefType type, const Def* def) override;
  void CreateEmpty(Entity entity) override;
  void CreateFromRenderDef(Entity entity, const RenderDef& def) override;
  void PostCreateInit(Entity entity, DefType type, const Def* def) override;
  void Destroy(Entity entity) override;

  FontPtr LoadFonts(const std::vector<std::string>& names) override;

  void SetFont(Entity entity, FontPtr font) override;

  const std::string* GetText(Entity entity) const override;
  void SetText(Entity entity, const std::string& text) override;

  void SetFontSize(Entity entity, float size) override;

  void SetBounds(Entity entity, const mathfu::vec2& bounds) override;

  void SetWrapMode(Entity entity, TextWrapMode wrap_mode) override;

  void SetEllipsis(Entity entity, const std::string& ellipsis) override;

  void SetHorizontalAlignment(Entity entity,
                              HorizontalAlignment horizontal) override;
  void SetVerticalAlignment(Entity entity, VerticalAlignment vertical) override;

  void SetTextDirection(TextDirection direction) override;

  const std::vector<LinkTag>* GetLinkTags(Entity entity) const override;
  const std::vector<mathfu::vec3>* GetCaretPositions(
      Entity entity) const override;

  bool IsTextReady(Entity entity) const override;

  void ProcessTasks() override;
  void WaitForAllTasks() override;

 private:
  // Task to generate a text buffer in a worker thread.
  class GenerateTextBufferTask {
   public:
    GenerateTextBufferTask(Entity target_entity, Entity desired_size_source,
                           const FontPtr& font, const std::string& text,
                           const TextBufferParams& params);

    Entity GetTarget() const { return target_entity_; }

    Entity GetDesiredSizeSource() const { return desired_size_source_; }

    // Called on a worker thread, this initializes the text buffer.
    void Process();

    // Called on the host thread, this performs post-processing such as
    // deformation (if applicable).
    void Finalize();

    const TextBufferPtr& GetOutputTextBuffer() const {
      return output_text_buffer_;
    }

   private:
    Entity target_entity_;
    Entity desired_size_source_;
    FontPtr font_;
    std::string text_;
    TextBufferParams params_;
    TextBufferPtr text_buffer_;
    TextBufferPtr output_text_buffer_;
  };

  using TaskPtr = std::shared_ptr<GenerateTextBufferTask>;
  using TaskQueue = AsyncProcessor<TaskPtr>;

  void GenerateText(Entity entity, Entity desired_size_source = kNullEntity);

  // Sets and activates |task.text_buffer_| on |component|.
  void SetTextBuffer(TextComponent* component, TextBufferPtr text_buffer,
                     Entity desired_size_source = kNullEntity);

  // Updates the text buffer with the results from any queued
  // GenerateTextBufferTasks.
  void UpdateTextBuffer(const TaskPtr& task);

  void EnqueueTask(TaskPtr task);

  Entity CreateEntity(TextComponent* component, const std::string& blueprint);
  void CreateTextEntities(TextComponent* component);
  void CreateLinkUnderlineEntity(TextComponent* component);
  void DestroyRenderEntities(TextComponent* component);
  void UpdateEntityUniforms(Entity entity, Entity source,
                            const mathfu::vec4& color);
  void UpdateUniforms(const TextComponent* component);

  void HideRenderEntities(const TextComponent& component);
  void ShowRenderEntities(const TextComponent& component);
  void AttachEvents(TextComponent* component);
  void OnDesiredSizeChanged(const DesiredSizeChangedEvent& event);

  // Main interface into the flatui text system.  Any memory/objects which are
  // managed by font_manager_ but held with a reference should be released
  // before FontManager is deleted, because some of the cleanup code in those
  // objects attempts to lock a lock in the FontManager and can deadlock if
  // FontManager is already gone.  This happened with the completed_tasks_
  // array because internally it holds on to flatui::FontBuffers, for example.
  std::unique_ptr<flatui::FontManager> font_manager_;

  // Dimensions of a single 2D glyph cache slice.
  mathfu::vec2i glyph_cache_size_ =
      mathfu::vec2i(flatui::kGlyphCacheWidth, flatui::kGlyphCacheHeight);

  // Max number of glyph cache slices. Flatui's default is 4, but given how
  // disastrous flushing the cache is, and since these aren't all pre-allocated,
  // we'll set our default limit higher.
  int max_glyph_cache_slices_ = 8;

  // Stores all text data, indexed by entity.
  ComponentPool<TextComponent> components_;

  // List of completed tasks that are waiting on a glyph texture update.
  std::vector<TaskPtr> completed_tasks_;

  // List of text buffer generation tasks.
  TaskQueue task_queue_;

  // Number of pending tasks.
  int num_pending_tasks_ = 0;

  // FPL renderer.  This is needed by flatui to query device support during
  // texture creation.  We don't use it for any rendering, so it can coexist
  // with RenderSystemFpl's.
  fplbase::Renderer renderer_;

  // Flag to track if hyphenation data has been initialized.
  bool hyphenation_initialized_ = false;

  // Global text direction that applies to text entities to be created.
  TextDirection text_direction_ = TextDirection_LeftToRight;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_TEXT_FLATUI_FLATUI_TEXT_SYSTEM_H_
