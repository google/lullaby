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

#include "lullaby/systems/text/flatui/flatui_text_system.h"

#include "fplbase/glplatform.h"
#include "fplbase/internal/type_conversions_gl.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/events/render_events.h"
#include "lullaby/events/text_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/contrib/deform/deform_system.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/contrib/layout/layout_box_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/text/detail/util.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/android_context.h"
#include "lullaby/util/filename.h"
#include "lullaby/util/string_preprocessor.h"
#include "lullaby/util/trace.h"
#include "lullaby/generated/text_def_generated.h"

namespace lull {
namespace {

constexpr int kDefaultPoolSize = 16;
constexpr HashValue kTextDefHash = ConstHash("TextDef");

// TODO: Use the same default as the text.
// TODO Remove non-blueprint default entities.
const mathfu::vec4 kDefaultLinkColor(39.0f / 255.0f, 121.0f / 255.0f, 1.0f,
                                     1.0f);  // "#2779FF";
const mathfu::vec4 kDefaultUnderlineSdfParams(1, 0, 0, 1);
constexpr float kDefaultUnderlineTexCoordAaPadding = 0.75f;

constexpr char kDefaultLinkTextBlueprint[] = ":DefaultLinkText:";
constexpr char kDefaultLinkUnderlineBlueprint[] = ":DefaultLinkUnderline:";

constexpr char kTexCoordAaPaddingUniform[] = "tex_coord_aa_padding";
constexpr char kSdfParamsUniform[] = "sdf_params";
constexpr char kTextureSizeUniform[] = "texture_size";
constexpr char kColorUniform[] = "color";

constexpr float kMetersFromMillimeters = .001f;

// Flatui sdf textures have white glyphs.
constexpr float kSdfDistOffset = 0.0f;
constexpr float kSdfDistScale = 1.0f;

constexpr int32_t kSmallGlyphSize = 32;
constexpr int32_t kNominalGlyphSize = 64;
constexpr int32_t kHugeGlyphSize = 128;

// Default android hyphenation pattern path.
// TODO Build hyphenation data and figure out directory for other
// platforms.
constexpr char kHyphenationPatternPath[] = "/system/usr/hyphen-data";

HashValue GetRenderPass(const RenderSystem& render_system, Entity entity) {
  // We only create 1 component so just take the first one.
  const auto passes = render_system.GetRenderPasses(entity);
  return passes.empty() ? RenderSystem::kDefaultPass : passes[0];
}

Entity CreateDefaultEntity(Registry* registry, Entity parent) {
  auto* entity_factory = registry->Get<EntityFactory>();
  const Entity entity = entity_factory->Create();

  auto* transform_system = registry->Get<TransformSystem>();
  transform_system->Create(entity, Sqt());

  auto* render_system = registry->Get<RenderSystem>();
  render_system->Create(entity, GetRenderPass(*render_system, parent));
  render_system->SetGroupId(entity, render_system->GetGroupId(parent));
  render_system->SetShader(entity, render_system->GetShader(parent));

  // Initialize the child with all the parent's uniforms and default color.
  render_system->CopyUniforms(entity, parent);
  render_system->SetDefaultColor(entity,
                                 render_system->GetDefaultColor(parent));

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

void CopyAlpha(Registry* registry, Entity entity, Entity parent) {
  auto* render_system = registry->Get<RenderSystem>();
  mathfu::vec4 parent_color = kDefaultLinkColor;
  mathfu::vec4 color = kDefaultLinkColor;
  render_system->GetColor(parent, &parent_color);
  render_system->GetColor(entity, &color);
  color.w = parent_color.w;
  render_system->SetColor(entity, color);
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
#ifdef __ANDROID__
  if (fplbase::GetAAssetManager() == nullptr) {
    auto* context = registry->Get<AndroidContext>();
    if (context) {
      // fplbase::AndroidSetJavaVM(vm, JNI_VERSION_1_6);
      fplbase::SetAAssetManager(context->GetAndroidAssetManager());
    }
  }
#endif

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
  TextTaskPtr task = nullptr;
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
  if (text_def.underline_padding()) {
    mathfu::vec2 underline_padding;
    MathfuVec2FromFbVec2(text_def.underline_padding(), &underline_padding);
    component->text_buffer_params.underline_padding = underline_padding;
  }
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

  auto* render_system = registry_->Get<RenderSystem>();
  const HashValue pass = GetRenderPass(*render_system, entity);
  render_system->SetUniformChangedCallback(
      entity, pass,
      [=](int submesh_index, string_view name, ShaderDataType type,
          Span<uint8_t> data, int count) {
        UpdateComponentUniform(entity, pass, submesh_index, name, type, data,
                               count);
      });
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
  LULLABY_CPU_TRACE("FlatuiTasks");

  for (const auto& entry : update_map_) {
    GenerateText(entry.first, entry.second);
  }
  update_map_.clear();

  TextTaskPtr task = nullptr;
  // Dequeue all completed tasks, but only apply the newest for each entity.
  while (DequeueTask(&task)) {
    if (task) {
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
    for (const TextTaskPtr& completed_task : completed_tasks_) {
      UpdateTextBuffer(completed_task);
    }
    completed_tasks_.clear();
  }
}

void FlatuiTextSystem::WaitForAllTasks() {
  while (!update_map_.empty() || num_pending_tasks_ > 0 ||
         !completed_tasks_.empty()) {
    ProcessTasks();
  }
}

void FlatuiTextSystem::EnqueueTask(TextComponent* component, TextTaskPtr task) {
  if (component->task_id != TextTaskQueue::kInvalidTaskId) {
    task_queue_.Cancel(component->task_id);
  }
  component->task = task;
  component->task_id = task_queue_.Enqueue(
      std::move(task), [](TextTaskPtr* task) { (*task)->Process(); });
  ++num_pending_tasks_;
}

bool FlatuiTextSystem::DequeueTask(TextTaskPtr* task) {
  task->reset();
  TextTaskPtr dequeued;
  if (!task_queue_.Dequeue(&dequeued)) {
    return false;
  }
  --num_pending_tasks_;

  TextComponent* component = components_.Get(dequeued->GetTarget());
  if (!component) {
    return true;
  }

  if (component->task != dequeued) {
    return true;
  }

  component->task.reset();
  component->task_id = TextTaskQueue::kInvalidTaskId;
  dequeued->Finalize();
  if (!dequeued->GetOutputTextBuffer()) {
    return true;
  }

  *task = std::move(dequeued);
  return true;
}

void FlatuiTextSystem::SetFont(Entity entity, FontPtr font) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    component->font = font;
    update_map_[entity] = kNullEntity;
  }
}

const std::string* FlatuiTextSystem::GetText(Entity entity) const {
  const TextComponent* component = components_.Get(entity);
  return (component ? &component->text : nullptr);
}

const std::string* FlatuiTextSystem::GetRenderedText(Entity entity) const {
  const TextComponent* component = components_.Get(entity);
  return component ? &component->rendered_text : nullptr;
}

void FlatuiTextSystem::SetText(Entity entity, const std::string& text) {
  TextComponent* component = components_.Get(entity);
  if (!component) {
    return;
  }

  // Store the unprocessed text.
  component->text = text;

  update_map_[entity] = kNullEntity;
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
  component->rendered_text =
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
  EnqueueTask(component, TextTaskPtr(new TextTask(entity, desired_size_source,
                                                  component->font,
                                                  component->rendered_text,
                                                  params)));
}

void FlatuiTextSystem::ReprocessAllText() {
  components_.ForEach([this](const TextComponent& component) {
    update_map_[component.GetEntity()] = kNullEntity;
  });
}

void FlatuiTextSystem::SetFontSize(Entity entity, float size) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    component->text_buffer_params.font_size = size;
    update_map_[entity] = kNullEntity;
  }
}

void FlatuiTextSystem::SetLineHeightScale(Entity entity,
                                          float line_height_scale) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    component->text_buffer_params.line_height_scale = line_height_scale;
    update_map_[entity] = kNullEntity;
  }
}

void FlatuiTextSystem::SetBounds(Entity entity, const mathfu::vec2& bounds) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    component->text_buffer_params.bounds = bounds;
    update_map_[entity] = kNullEntity;
  }
}

void FlatuiTextSystem::SetWrapMode(Entity entity, TextWrapMode wrap_mode) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    component->text_buffer_params.wrap_mode = wrap_mode;
    update_map_[entity] = kNullEntity;
  }
}

void FlatuiTextSystem::SetEllipsis(Entity entity, const std::string& ellipsis) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    component->text_buffer_params.ellipsis = ellipsis;
    update_map_[entity] = kNullEntity;
  }
}

void FlatuiTextSystem::SetHorizontalAlignment(Entity entity,
                                              HorizontalAlignment horizontal) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    component->text_buffer_params.horizontal_align = horizontal;
    update_map_[entity] = kNullEntity;
  }
}

void FlatuiTextSystem::SetVerticalAlignment(Entity entity,
                                            VerticalAlignment vertical) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    component->text_buffer_params.vertical_align = vertical;
    update_map_[entity] = kNullEntity;
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

void FlatuiTextSystem::SetTextDirection(Entity entity,
                                        TextDirection direction) {
  TextComponent* component = components_.Get(entity);
  if (component) {
    component->text_buffer_params.direction = direction;
    update_map_[entity] = kNullEntity;
  }
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

void FlatuiTextSystem::UpdateTextBuffer(const TextTaskPtr& task) {
  auto* component = components_.Get(task->GetTarget());
  if (component && task->GetOutputTextBuffer()) {
    SetTextBuffer(component, task->GetOutputTextBuffer(),
                  task->GetDesiredSizeSource());
  }
}

Entity FlatuiTextSystem::CreateEntity(TextComponent* component,
                                      const std::string& blueprint) {
  const Entity parent = component->GetEntity();

  // TODO Remove non-blueprint default entities.
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
    if (component->text_buffer_params.underline_padding) {
      // Increase the minimum width of the underline.
      render_system->SetUniform(entity, kTexCoordAaPaddingUniform,
                                &kDefaultUnderlineTexCoordAaPadding, 1, 1);
    }
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
      CopyAlpha(registry_, entity, component->GetEntity());
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
    CopyAlpha(registry_, entity, component->GetEntity());
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

void FlatuiTextSystem::UpdateComponentUniform(Entity entity, HashValue pass,
                                              int submesh_index,
                                              string_view name,
                                              ShaderDataType type,
                                              Span<uint8_t> data, int count) {
  const TextComponent* component = components_.Get(entity);
  if (!component) {
    return;
  }
  if (name == kTexCoordAaPaddingUniform || name == kSdfParamsUniform ||
      name == kTextureSizeUniform) {
    return;
  }

  auto* render_system = registry_->Get<RenderSystem>();
  const bool is_color =
      type == ShaderDataType_Float4 && count == 1 && name == kColorUniform;

  for (Entity plain_entity : component->plain_entities) {
    render_system->SetUniform({plain_entity, pass, submesh_index}, name, type,
                              data, count);
  }

  for (Entity link_entity : component->link_entities) {
    mathfu::vec4 color = kDefaultLinkColor;
    if (is_color) {
      render_system->GetColor(link_entity, &color);
      color.w = data[3];
      data = SpanFromVector(color);
    }
    render_system->SetUniform({link_entity, pass, submesh_index}, name, type,
                              data, count);
  }

  if (component->underline_entity != kNullEntity) {
    mathfu::vec4 color = kDefaultLinkColor;
    if (is_color) {
      render_system->GetColor(component->underline_entity, &color);
      color.w = data[3];
      data = SpanFromVector(color);
    }
    render_system->SetUniform({component->underline_entity, pass, submesh_index},
                              name, type, data, count);
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

  if (component->loading_buffer) {
    return false;
  }

  if (update_map_.count(entity) > 0) {
    return false;
  }

  return true;
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
    update_map_[event.target] = event.source;
  }
}
}  // namespace lull
