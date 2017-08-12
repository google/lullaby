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

#include "lullaby/systems/text/flatui/flatui_text_system.h"

#include "fplbase/glplatform.h"
#include "fplbase/internal/type_conversions_gl.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/events/render_events.h"
#include "lullaby/events/text_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/file/file.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/systems/deform/deform_system.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/layout/layout_box_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/text/detail/util.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/string_preprocessor.h"
#include "lullaby/util/trace.h"
#include "lullaby/generated/text_def_generated.h"

namespace lull {
namespace {

constexpr int kDefaultPoolSize = 16;
const HashValue kTextDefHash = Hash("TextDef");

// TODO(b/32219426): Use the same default as the text.
// TODO(b/33705906) Remove non-blueprint default entities.
const mathfu::vec4 kDefaultLinkColor(39.0f / 255.0f, 121.0f / 255.0f, 1.0f,
                                     1.0f);  // "#2779FF";
const mathfu::vec4 kDefaultUnderlineSdfParams(1, 0, 0, 1);

constexpr const char* kDefaultLinkTextBlueprint = ":DefaultLinkText:";
constexpr const char* kDefaultLinkUnderlineBlueprint = ":DefaultLinkUnderline:";

constexpr const char* kSdfParamsUniform = "sdf_params";
constexpr const char* kTextureSizeUniform = "texture_size";

constexpr float kMetersFromMillimeters = .001f;

// Flatui sdf textures have white glyphs.
constexpr float kSdfDistOffset = 0.0f;
constexpr float kSdfDistScale = 1.0f;

constexpr int32_t kSmallGlyphSize = 32;
constexpr int32_t kNominalGlyphSize = 64;
constexpr int32_t kHugeGlyphSize = 128;

// Default android hyphenation pattern path.
// TODO(b/34112774) Build hyphenation data and figure out directory for other
// platforms.
constexpr const char* kHyphenationPatternPath = "/system/usr/hyphen-data";

Entity CreateDefaultEntity(Registry* registry, Entity parent) {
  auto* entity_factory = registry->Get<EntityFactory>();
  const Entity entity = entity_factory->Create();

  auto* transform_system = registry->Get<TransformSystem>();
  transform_system->Create(entity, Sqt());

  auto* render_system = registry->Get<RenderSystem>();
  render_system->Create(entity, render_system->GetRenderPass(parent));
  render_system->SetShader(entity, render_system->GetShader(parent));

  mathfu::vec4 color;
  if (render_system->GetColor(parent, &color)) {
    render_system->SetColor(entity, color);
  }

  // Add as a child after initializing the render component so that any stencil
  // settings from clip system are correctly applied.
  transform_system->AddChild(parent, entity);

  auto* deform_system = registry->Get<DeformSystem>();
  if (deform_system && deform_system->IsSetAsDeformed(parent)) {
    deform_system->SetAsDeformed(entity);
    DCHECK(deform_system->IsSetAsDeformed(entity));
  }

  return entity;
}

}  // namespace

TextSystem::TextSystem(Registry* registry)
    : TextSystem(registry, std::unique_ptr<TextSystemImpl>(
                               new FlatuiTextSystem(registry))) {
  RegisterDependency<RenderSystem>(this);
  RegisterDependency<TransformSystem>(this);
}

static int32_t GetGlyphSizeForTextSize(int32_t size) {
  if (size <= kSmallGlyphSize) {
    return kSmallGlyphSize;
  } else if (size <= kNominalGlyphSize) {
    return kNominalGlyphSize;
  } else {
    return kHugeGlyphSize;
  }
}

FlatuiTextSystem::FlatuiTextSystem(Registry* registry)
    : TextSystemImpl(registry), components_(kDefaultPoolSize) {
  // Initialize the renderer so it knows the device capabilities for flatui's
  // texture creation.
  renderer_.Initialize(/* window_size = */ mathfu::kZeros2i,
                       /* window_title = */ "");

  Dispatcher* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->Connect(this, [this](const DesiredSizeChangedEvent& event) {
    OnDesiredSizeChanged(event);
  });
}

FlatuiTextSystem::~FlatuiTextSystem() {
  Dispatcher* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->DisconnectAll(this);

  // We need to destroy all TextBuffers before flatui::FontManager's dtor, which
  // can be referenced in two places: the TextBuffer and completed tasks in the
  // task queue.

  // First, stop the queue and drain any completed tasks.
  task_queue_.Stop();
  TaskPtr task = nullptr;
  while (task_queue_.Dequeue(&task)) {
    task.reset();
  }

  // Next, release all TextBuffers held by the components.
  for (auto& component : components_) {
    component.buffer.reset();
  }

  // As commented above, explicitly release completed tasks so that TextBuffers
  // are release before flatui::FontManager is destroyed implicitly upon leaving
  // this function.  Otherwise a deadlock happens inside
  // FontManager::ReleaseBuffer().
  completed_tasks_.clear();
}

void FlatuiTextSystem::SetGlyphCacheSize(const mathfu::vec2i& size,
                                         int max_slices) {
  CHECK_GE(size.x, 64);
  CHECK_GE(size.y, 64);
  CHECK_GE(max_slices, 1);
  DCHECK(!font_manager_)
      << "SetGlyphCacheSize has no effect if called after Initialize.";

  glyph_cache_size_ = size;
  max_glyph_cache_slices_ = max_slices;
}

void FlatuiTextSystem::Initialize() {
  std::unique_ptr<flatui::FontManager> font_manager(
      new flatui::FontManager(glyph_cache_size_, max_glyph_cache_slices_));

  font_manager->SetSizeSelector(
      [](int32_t size) { return GetGlyphSizeForTextSize(size); });

  font_manager_ = std::move(font_manager);
}

FontPtr FlatuiTextSystem::LoadFonts(const std::vector<std::string>& names) {
  CHECK(font_manager_);
  return std::make_shared<Font>(font_manager_.get(), names);
}

void FlatuiTextSystem::Create(Entity entity, DefType type, const Def* def) {
  if (type != kTextDefHash) {
    LOG(DFATAL) << "Invalid type passed to Create.  Expecting TextDef!";
    return;
  }

  const TextDef& text_def = *ConvertDef<TextDef>(def);
  if (!text_def.fonts()) {
    LOG(DFATAL) << "No font specified in TextDef";
    return;
  }

  FontPtr font = TextSystemImpl::LoadFonts(text_def.fonts());

  TextComponent* component = components_.Emplace(entity);
  if (!component) {
    LOG(DFATAL) << "Entity already has a text component";
    return;
  }

  component->font = font;
  component->text_buffer_params.font_size =
      std::max(text_def.font_size(), text_def.line_height());
  component->text_buffer_params.line_height_scale =
      text_def.line_height_scale();
  component->text_buffer_params.kerning_scale = text_def.kerning_scale();
  component->edge_softness = text_def.edge_softness();
  MathfuVec2FromFbVec2(text_def.bounds(),
                       &component->text_buffer_params.bounds);
  component->text_buffer_params.horizontal_align =
      text_def.horizontal_alignment();
  component->text_buffer_params.vertical_align = text_def.vertical_alignment();
  if (text_def.direction() == TextDirection_UseSystemSetting) {
    component->text_buffer_params.direction = text_direction_;
  } else {
    component->text_buffer_params.direction = text_def.direction();
  }
  component->text_buffer_params.html_mode = text_def.html_mode();
  if (text_def.ellipsis()) {
    component->text_buffer_params.ellipsis = text_def.ellipsis()->str();
  }
  component->text_buffer_params.wrap_mode = text_def.wrap_mode();

  if (text_def.link_text_blueprint()) {
    component->link_text_blueprint = text_def.link_text_blueprint()->str();
  } else if (component->text_buffer_params.html_mode ==
             TextHtmlMode_ExtractLinks) {
    LOG(INFO) << "Text component extracts HTML links but has no link text "
                 "blueprint; using default.";
    component->link_text_blueprint = kDefaultLinkTextBlueprint;
  }

  if (text_def.link_underline_blueprint()) {
    component->link_underline_blueprint =
        text_def.link_underline_blueprint()->str();
  } else if (component->text_buffer_params.html_mode ==
             TextHtmlMode_ExtractLinks) {
    LOG(INFO) << "Text component extracts HTML links but has no link "
                 "underline blueprint; using default.";
    component->link_underline_blueprint = kDefaultLinkUnderlineBlueprint;
  }

  AttachEvents(component);

  // Delay reading hyphenation data until it's needed.  Reduces file loads
  // for apps that don't need it and helps keeps tests running.
  if (component->text_buffer_params.wrap_mode == TextWrapMode_Hyphenate &&
      !hyphenation_initialized_) {
    font_manager_->SetupHyphenationPatternPath(kHyphenationPatternPath);
    hyphenation_initialized_ = true;
  }
}

void FlatuiTextSystem::PostCreateInit(Entity entity, DefType type,
                                      const Def* def) {
  if (type != kTextDefHash) {
    LOG(DFATAL) << "Invalid type passed to Create.  Expecting TextDef!";
    return;
  }

  const auto& data = *ConvertDef<TextDef>(def);
  if (data.text()) {
    SetText(entity, data.text()->c_str());
  }
}

void FlatuiTextSystem::CreateEmpty(Entity entity) {
  TextComponent* component = components_.Emplace(entity);
  if (!component) {
    LOG(DFATAL) << "Entity already has a text component";
    return;
  }

  TextDefT default_def;  // Pull our defaults from the fbs file.

  component->text_buffer_params.font_size = default_def.font_size;
  component->text_buffer_params.line_height_scale =
      default_def.line_height_scale;
  component->text_buffer_params.kerning_scale = default_def.kerning_scale;
  component->edge_softness = default_def.edge_softness;
  component->text_buffer_params.bounds = default_def.bounds;
  component->text_buffer_params.horizontal_align =
      default_def.horizontal_alignment;
  component->text_buffer_params.vertical_align = default_def.vertical_alignment;
  if (default_def.direction == TextDirection_UseSystemSetting) {
    component->text_buffer_params.direction = text_direction_;
  } else {
    component->text_buffer_params.direction = default_def.direction;
  }
  component->text_buffer_params.html_mode = default_def.html_mode;
  component->text_buffer_params.ellipsis = default_def.ellipsis;
  component->text_buffer_params.wrap_mode = default_def.wrap_mode;
  component->link_text_blueprint = default_def.link_text_blueprint;
  component->link_underline_blueprint = default_def.link_underline_blueprint;

  AttachEvents(component);
}

void FlatuiTextSystem::CreateFromRenderDef(Entity entity,
                                           const RenderDef& render_def) {
  CHECK(render_def.font() != nullptr);

  const FontDef& font_def = *render_def.font();
  FontPtr font;
  if (font_def.fonts()) {
    font = TextSystemImpl::LoadFonts(font_def.fonts());
  } else {
    std::string filename = font_def.font()->str();
    if (!EndsWith(filename, ".ttf")) {
      filename.append(".ttf");
    }
    font = LoadFonts({filename});
  }

  TextComponent* component = components_.Emplace(entity);
  if (!component) {
    LOG(DFATAL) << "Entity already has a text component";
    return;
  }

  component->font = font;
  component->text_buffer_params.font_size =
      kMetersFromMillimeters * static_cast<float>(font_def.size());
  component->text_buffer_params.line_height_scale =
      font_def.line_height_scale();
  component->text_buffer_params.kerning_scale = font_def.kerning_scale();
  component->edge_softness = font_def.edge_softness();
  component->text_buffer_params.bounds =
      kMetersFromMillimeters *
      mathfu::vec2(
          mathfu::vec2i(font_def.rect_width(), font_def.rect_height()));
  component->text_buffer_params.horizontal_align =
      font_def.horizontal_alignment();
  component->text_buffer_params.vertical_align = font_def.vertical_alignment();
  component->text_buffer_params.direction = text_direction_;
  component->text_buffer_params.html_mode = font_def.parse_and_strip_html()
                                                ? TextHtmlMode_ExtractLinks
                                                : TextHtmlMode_Ignore;
  if (font_def.ellipsis()) {
    component->text_buffer_params.ellipsis = font_def.ellipsis()->str();
  }
  component->text_buffer_params.wrap_mode =
      font_def.wrap_content() ? TextWrapMode_BetweenWords : TextWrapMode_None;

  component->link_text_blueprint = kDefaultLinkTextBlueprint;
  component->link_underline_blueprint = kDefaultLinkUnderlineBlueprint;

  AttachEvents(component);
}

void FlatuiTextSystem::Destroy(Entity entity) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    DestroyRenderEntities(component);
    components_.Destroy(entity);
  }
}

void FlatuiTextSystem::ProcessTasks() {
  LULLABY_CPU_TRACE_CALL();

  TaskPtr task = nullptr;
  while (task_queue_.Dequeue(&task)) {
    --num_pending_tasks_;
    task->Finalize();
    const Entity entity = task->GetTarget();
    TextComponent* component = components_.Get(entity);
    if (component && task->GetOutputTextBuffer()) {
      // The font geometry is ready but the texture atlas might not be ready,
      // so move the font buffer to the completed tasks buffer until the atlas
      // is ready.  We can't rely just on OutputTextBuffer()->IsReady(), as that
      // doesn't assure that the texture has been assigned an ID yet.
      completed_tasks_.push_back(task);
    }
  }

  if (font_manager_->StartRenderPass()) {
    // Once we have successfully started the font render pass we can assume that
    // the font texture atlases have been successfully updated and it is now
    // safe to render fonts.
    for (const TaskPtr& completed_task : completed_tasks_) {
      UpdateTextBuffer(completed_task);
    }
    completed_tasks_.clear();
  }

  for (auto& component : components_) {
    UpdateUniforms(&component);
  }
}

void FlatuiTextSystem::WaitForAllTasks() {
  while (num_pending_tasks_ > 0 || !completed_tasks_.empty()) {
    ProcessTasks();
  }
}

void FlatuiTextSystem::EnqueueTask(TaskPtr task) {
  task_queue_.Enqueue(std::move(task),
                      [](TaskPtr* task) { (*task)->Process(); });
  ++num_pending_tasks_;
}

void FlatuiTextSystem::SetFont(Entity entity, FontPtr font) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    component->font = font;
  }
}

const std::string* FlatuiTextSystem::GetText(Entity entity) const {
  const TextComponent* component = components_.Get(entity);
  return (component ? &component->text : nullptr);
}

void FlatuiTextSystem::SetText(Entity entity, const std::string& text) {
  TextComponent* component = components_.Get(entity);
  if (!component) {
    return;
  }

  // Store the unprocessed text.
  component->text = text;

  GenerateText(entity);
}

void FlatuiTextSystem::GenerateText(Entity entity, Entity desired_size_source) {
  TextComponent* component = components_.Get(entity);
  if (!component) {
    return;
  }

  if (component->text.empty()) {
    SetTextBuffer(component, nullptr);
    return;
  }

  auto* preprocessor = registry_->Get<StringPreprocessor>();
  const std::string processed_text =
      preprocessor ? preprocessor->ProcessString(component->text)
                   : component->text;

  component->loading_buffer = true;

  // Create a copy of params and override with desired_size if set so that
  // original params are unchanged.
  TextBufferParams params = component->text_buffer_params;
  if (desired_size_source != kNullEntity) {
    auto* layout_box_system = registry_->Get<LayoutBoxSystem>();
    Optional<float> x = layout_box_system->GetDesiredSizeX(entity);
    Optional<float> y = layout_box_system->GetDesiredSizeY(entity);
    if (x) {
      params.bounds.x = *x;
    }
    if (y) {
      params.bounds.y = *y;
    }
  }
  EnqueueTask(TaskPtr(new GenerateTextBufferTask(
      entity, desired_size_source, component->font, processed_text, params)));
}

void FlatuiTextSystem::SetFontSize(Entity entity, float size) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    component->text_buffer_params.font_size = size;
  }
}

void FlatuiTextSystem::SetBounds(Entity entity, const mathfu::vec2& bounds) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    component->text_buffer_params.bounds = bounds;
  }
}

void FlatuiTextSystem::SetWrapMode(Entity entity, TextWrapMode wrap_mode) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    component->text_buffer_params.wrap_mode = wrap_mode;
  }
}

void FlatuiTextSystem::SetEllipsis(Entity entity, const std::string& ellipsis) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    component->text_buffer_params.ellipsis = ellipsis;
  }
}

void FlatuiTextSystem::SetHorizontalAlignment(Entity entity,
                                              HorizontalAlignment horizontal) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    component->text_buffer_params.horizontal_align = horizontal;
  }
}

void FlatuiTextSystem::SetVerticalAlignment(Entity entity,
                                            VerticalAlignment vertical) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    component->text_buffer_params.vertical_align = vertical;
  }
}

void FlatuiTextSystem::SetTextDirection(TextDirection direction) {
  if (direction == TextDirection_UseSystemSetting) {
    LOG(INFO) << "Ignoring text direction: UseSystemSetting. Specify either "
                 "LeftToRight or RightToLeft.";
    return;
  }
  text_direction_ = direction;
}

const std::vector<LinkTag>* FlatuiTextSystem::GetLinkTags(Entity entity) const {
  const TextComponent* component = components_.Get(entity);
  return component && component->buffer ? &component->buffer->GetLinks()
                                        : nullptr;
}

void FlatuiTextSystem::SetTextBuffer(TextComponent* component,
                                     TextBufferPtr text_buffer,
                                     Entity desired_size_source) {
  DestroyRenderEntities(component);

  component->loading_buffer = false;
  component->buffer = std::move(text_buffer);

  const Entity entity = component->GetEntity();

  auto* transform_system = registry_->Get<TransformSystem>();
  auto* layout_box_system = registry_->Get<LayoutBoxSystem>();
  if (component->buffer) {
    transform_system->SetAabb(entity, component->buffer->GetAabb());

    if (layout_box_system) {
      if (desired_size_source != kNullEntity) {
        layout_box_system->SetActualBox(entity, desired_size_source,
                                        component->buffer->GetAabb());
      } else {
        layout_box_system->SetOriginalBox(entity, component->buffer->GetAabb());
      }
    }

    CreateTextEntities(component);
    CreateLinkUnderlineEntity(component);

    // If our entity is already hidden, hide its newly-created render entities.
    const auto* render_system = registry_->Get<RenderSystem>();
    if (render_system->IsHidden(entity)) {
      HideRenderEntities(*component);
    }
  } else {
    transform_system->SetAabb(entity, Aabb());
    if (layout_box_system) {
      layout_box_system->SetOriginalBox(entity, Aabb());
    }
  }

  // Since we're using all separate entities for our text rendering, the
  // render system won't send out ready events for the main entity, which is
  // what other systems are listening for.
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  if (dispatcher_system) {
    dispatcher_system->Send(entity, TextReadyEvent(entity));
  }
}

void FlatuiTextSystem::UpdateTextBuffer(const TaskPtr& task) {
  auto* component = components_.Get(task->GetTarget());
  if (component && task->GetOutputTextBuffer()) {
    SetTextBuffer(component, task->GetOutputTextBuffer(),
                  task->GetDesiredSizeSource());
  }
}

Entity FlatuiTextSystem::CreateEntity(TextComponent* component,
                                      const std::string& blueprint) {
  const Entity parent = component->GetEntity();

  // TODO(b/33705906) Remove non-blueprint default entities.
  Entity entity;
  if (blueprint == kDefaultLinkTextBlueprint) {
    entity = CreateDefaultEntity(registry_, parent);

    auto* render_system = registry_->Get<RenderSystem>();
    render_system->SetColor(entity, kDefaultLinkColor);
  } else if (blueprint == kDefaultLinkUnderlineBlueprint) {
    entity = CreateDefaultEntity(registry_, parent);

    // The default underline entities reuse the text shader, so bind a white
    // texture and set sdf params to effectively render a solid quad.
    auto* render_system = registry_->Get<RenderSystem>();
    render_system->SetColor(entity, kDefaultLinkColor);
    render_system->SetTexture(entity, 0, render_system->GetWhiteTexture());
    render_system->SetUniform(entity, kSdfParamsUniform,
                              &kDefaultUnderlineSdfParams[0], 4, 1);
  } else {
    auto* transform_system = registry_->Get<TransformSystem>();
    entity = transform_system->CreateChild(parent, blueprint);
  }

  return entity;
}

void FlatuiTextSystem::CreateTextEntities(TextComponent* component) {
  const size_t num_slices = component->buffer->GetNumSlices();

  const int32_t text_size_mm = static_cast<int>(
      component->text_buffer_params.font_size / kMetersFromMillimeters);
  const float softness_scale =
      static_cast<float>(GetGlyphSizeForTextSize(text_size_mm)) *
      kMetersFromMillimeters / component->text_buffer_params.font_size;
  const mathfu::vec4 sdf_params = CalcSdfParams(
      component->edge_softness * softness_scale, kSdfDistOffset, kSdfDistScale);
  auto* render_system = registry_->Get<RenderSystem>();
  for (size_t i = 0; i < num_slices; ++i) {
    Entity entity;
    if (component->buffer->IsLinkSlice(i)) {
      entity = CreateEntity(component, component->link_text_blueprint);
      if (entity == kNullEntity) {
        continue;
      }
      component->link_entities.emplace_back(entity);
    } else {
      entity = CreateDefaultEntity(registry_, component->GetEntity());
      component->plain_entities.emplace_back(entity);
    }

    const fplbase::Texture* texture = component->buffer->GetSliceTexture(i);
    render_system->SetAndDeformMesh(entity,
                                    component->buffer->BuildSliceMesh(i));
    render_system->SetTextureId(entity, 0, GL_TEXTURE_2D,
                                fplbase::GlTextureHandle(texture->id()));
    const mathfu::vec2 texture_size = mathfu::vec2(texture->size());
    render_system->SetUniform(entity, kTextureSizeUniform, &texture_size[0], 2,
                              1);
    render_system->SetUniform(entity, kSdfParamsUniform, &sdf_params[0], 4, 1);
  }
}

void FlatuiTextSystem::CreateLinkUnderlineEntity(TextComponent* component) {
  if (component->text_buffer_params.html_mode != TextHtmlMode_ExtractLinks ||
      component->buffer->GetUnderlineVertices().empty() ||
      component->link_underline_blueprint.empty()) {
    return;
  }

  const Entity entity =
      CreateEntity(component, component->link_underline_blueprint);
  if (entity != kNullEntity) {
    auto* render_system = registry_->Get<RenderSystem>();
    render_system->SetAndDeformMesh(entity,
                                    component->buffer->BuildUnderlineMesh());
    component->underline_entity = entity;
  }
}

void FlatuiTextSystem::DestroyRenderEntities(TextComponent* component) {
  auto* entity_factory = registry_->Get<EntityFactory>();

  for (const auto& e : component->plain_entities) {
    entity_factory->Destroy(e);
  }
  component->plain_entities.clear();

  for (const auto& e : component->link_entities) {
    entity_factory->Destroy(e);
  }
  component->link_entities.clear();

  if (component->underline_entity != kNullEntity) {
    entity_factory->Destroy(component->underline_entity);
    component->underline_entity = kNullEntity;
  }
}

void FlatuiTextSystem::UpdateEntityUniforms(Entity entity, Entity source,
                                            const mathfu::vec4& color) {
  auto* render_system = registry_->Get<RenderSystem>();
  float sdf_params[4];
  const bool have_sdf_params =
      render_system->GetUniform(entity, kSdfParamsUniform, 4, sdf_params);
  float texture_size[2];
  const bool have_texture_size =
      render_system->GetUniform(entity, kTextureSizeUniform, 2, texture_size);

  render_system->CopyUniforms(entity, source);

  // Color must be set separately, since links have a different color than
  // regular text.
  render_system->SetColor(entity, color);

  if (have_sdf_params) {
    render_system->SetUniform(entity, kSdfParamsUniform, &sdf_params[0], 4, 1);
  }

  if (have_texture_size) {
    render_system->SetUniform(entity, kTextureSizeUniform, &texture_size[0], 2,
                              1);
  }
}

void FlatuiTextSystem::UpdateUniforms(const TextComponent* component) {
  auto* render_system = registry_->Get<RenderSystem>();
  const Entity source = component->GetEntity();
  mathfu::vec4 source_color;
  mathfu::vec4 plain_color = mathfu::kOnes4f;
  if (render_system->GetColor(source, &source_color)) {
    plain_color = source_color;
  } else {
    source_color = mathfu::kOnes4f;
  }

  for (Entity entity : component->plain_entities) {
    UpdateEntityUniforms(entity, source, plain_color);
  }

  for (Entity entity : component->link_entities) {
    mathfu::vec4 color;
    if (!render_system->GetColor(entity, &color)) {
      color = kDefaultLinkColor;
    }
    color.w = source_color.w;
    UpdateEntityUniforms(entity, source, color);
  }

  if (component->underline_entity != kNullEntity) {
    mathfu::vec4 color;
    if (!render_system->GetColor(component->underline_entity, &color)) {
      color = kDefaultLinkColor;
    }
    color.w = source_color.w;
    UpdateEntityUniforms(component->underline_entity, source, color);
  }
}

const std::vector<mathfu::vec3>* FlatuiTextSystem::GetCaretPositions(
    Entity entity) const {
  const TextComponent* component = components_.Get(entity);
  if (!component || !component->buffer) {
    return nullptr;
  }
  return &component->buffer->GetCaretPositions();
}

bool FlatuiTextSystem::IsTextReady(Entity entity) const {
  const TextComponent* component = components_.Get(entity);
  if (!component) {
    // If we don't have anything associated with the entity, then we don't have
    // any text that isn't ready.
    return true;
  }

  return !component->loading_buffer;
}

void FlatuiTextSystem::HideRenderEntities(const TextComponent& component) {
  auto* render_system = registry_->Get<RenderSystem>();

  for (Entity entity : component.plain_entities) {
    render_system->Hide(entity);
  }

  for (Entity entity : component.link_entities) {
    render_system->Hide(entity);
  }

  if (component.underline_entity != kNullEntity) {
    render_system->Hide(component.underline_entity);
  }
}

void FlatuiTextSystem::ShowRenderEntities(const TextComponent& component) {
  auto* render_system = registry_->Get<RenderSystem>();

  for (Entity entity : component.plain_entities) {
    render_system->Show(entity);
  }

  for (Entity entity : component.link_entities) {
    render_system->Show(entity);
  }

  if (component.underline_entity != kNullEntity) {
    render_system->Show(component.underline_entity);
  }
}

void FlatuiTextSystem::AttachEvents(TextComponent* component) {
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  if (!dispatcher_system) {
    return;
  }

  const Entity entity = component->GetEntity();

  component->on_hidden =
      dispatcher_system->Connect(entity, [this](const HiddenEvent& event) {
        const TextComponent* component = components_.Get(event.entity);
        if (component) {
          HideRenderEntities(*component);
        }
      });
  component->on_unhidden =
      dispatcher_system->Connect(entity, [this](const UnhiddenEvent& event) {
        const TextComponent* component = components_.Get(event.entity);
        if (component) {
          ShowRenderEntities(*component);
        }
      });
}

void FlatuiTextSystem::OnDesiredSizeChanged(
    const DesiredSizeChangedEvent& event) {
  const TextComponent* component = components_.Get(event.target);
  if (component) {
    GenerateText(event.target, event.source);
  }
}

FlatuiTextSystem::GenerateTextBufferTask::GenerateTextBufferTask(
    Entity target_entity, Entity desired_size_source, const FontPtr& font,
    const std::string& text, const TextBufferParams& params)
    : target_entity_(target_entity),
      desired_size_source_(desired_size_source),
      font_(font),
      text_(text),
      params_(params),
      text_buffer_(nullptr),
      output_text_buffer_(nullptr) {}

void FlatuiTextSystem::GenerateTextBufferTask::Process() {
  if (font_ && font_->Bind()) {
    TextBufferPtr buffer =
        TextBuffer::Create(font_->GetFontManager(), text_, params_);
    text_buffer_ = std::move(buffer);
  } else {
    LOG(ERROR) << "Font is null or failed to bind in "
                  "GenerateTextBufferTask::Process()";
  }
}

void FlatuiTextSystem::GenerateTextBufferTask::Finalize() {
  if (text_buffer_) {
    // TODO(b/33705855) Remove Finalize.
    text_buffer_->Finalize();
  }
  output_text_buffer_ = text_buffer_;
}
}  // namespace lull
