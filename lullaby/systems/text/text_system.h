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

#ifndef LULLABY_SYSTEMS_TEXT_TEXT_SYSTEM_H_
#define LULLABY_SYSTEMS_TEXT_TEXT_SYSTEM_H_

#include <memory>
#include <string>
#include <vector>

#include "mathfu/glsl_mappings.h"

#include "lullaby/generated/render_def_generated.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/text/html_tags.h"
#include "lullaby/generated/text_def_generated.h"

namespace lull {

class Font;  // Font class is defined by the implementation.
using FontPtr = std::shared_ptr<Font>;

class TextSystemImpl;

// Various preprocessing modes for text rendered by the TextSystem.
// All text sent into the system will be mutated according to one of these
// modes.
enum TextSystemPreprocessingModes {
  kPreprocessingModeNone,
  kPreprocessingModeStringPreprocessor,  // Use lull::StringPreprocessor
};

// The TextSystem manages the rendering of i18n strings.  For each text string,
// SDF glyphs are generated and stored in textures, which are referenced by the
// corresponding meshes which are assigned to entities.
//
// Functions are declared as abstract to allow for multiple implementations:
// Flatui: Supports line wrapping and RTL layouts.
// Ion: No line wrapping or RTL support.  Allows existing Ion fonts to be used.
class TextSystem : public System {
 public:
  explicit TextSystem(Registry* registry);
  TextSystem(Registry* registry, std::unique_ptr<TextSystemImpl> impl);

  ~TextSystem() override;

  void Initialize() override;

  void Create(Entity entity, DefType type, const Def* def) override;

  void PostCreateInit(Entity entity, DefType type, const Def* def) override;

  void Destroy(Entity entity) override;

  // Creates an empty text component for |entity|.  The font, line height and
  // text values will need to be set before it will be visible.
  void CreateEmpty(Entity entity);

  // Creates an entity using an old FontDef (contained in a RenderDef).  This
  // function is deprecated, and is provided for compatibility only.
  void CreateFromRenderDef(Entity entity, const RenderDef& render_def);

  // Flatui: Loads a list of fonts.  Each glyph will check each font in the list
  // until it finds one that supports it.
  // Ion: Only the first font is loaded.
  FontPtr LoadFonts(const std::vector<std::string>& names);

  // A variant of LoadFonts(std::vector) that takes flatbuffers strings.
  FontPtr LoadFonts(
      const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>*
          names);

  // Sets |font| on |entity|.
  // TODO(b/37951300) Doesn't take effect until the next call to SetText.
  void SetFont(Entity entity, FontPtr font);

  // Returns |entity|'s current text value, or nullptr if it isn't registered.
  // This is the same value that was passed to SetText, ie it has not been
  // processed by StringProcessor.
  const std::string* GetText(Entity entity) const;

  // Updates |entity| to display |text|.  If there is a deformation function set
  // on |entity| in the RenderSystem then the mesh generation will be deferred
  // until ProcessTasks is called.
  //
  // Before the text mesh is generated, the text will be mutated according to
  // the specified TextPreprocessingMode.  The default mode uses
  // lull::StringPreprocessor to format the text.
  void SetText(Entity entity, const std::string& text,
               TextSystemPreprocessingModes preprocess =
                   kPreprocessingModeStringPreprocessor);

  // Sets |entity|'s font size to |size|, measured in meters. Takes effect on
  // the next call to SetText.  Bug to fix: TODO(b/37951300)
  void SetFontSize(Entity entity, float size);

  // DEPRECATED.  Do not use.
  void SetLineHeight(Entity entity, float height);

  // Sets |entity|'s rectangle area for rendering the text in meters.
  // If both the width and height are zero, the text will be displayed on a
  // single line.
  // If just the height is zero, the text will break to multiple lines based
  // on the TextWrapMode.
  // TODO(b/37951300) Doesn't take effect until the next call to SetText.
  void SetBounds(Entity entity, const mathfu::vec2& bounds);

  // Sets how |entity|'s text will wrap based on the text bounds.
  // See the documentation for TextWrapMode for info on how different values
  // affect the wrapping.
  // TODO(b/37951300) Doesn't take effect until the next call to SetText.
  void SetWrapMode(Entity entity, TextWrapMode wrap_mode);

  // Sets |entity|'s |ellipsis| string, which will be appended to the last of
  // the visible characters when text does not fit in its bounds rect. Takes
  // effect on the next call to SetText.  Bug to fix: TODO(b/37951300)
  void SetEllipsis(Entity entity, const std::string& ellipsis);

  // Sets |entity|'s |horizontal| alignment. Takes effect on the next call to
  // SetText.  Bug to fix: TODO(b/37951300)
  void SetHorizontalAlignment(Entity entity, HorizontalAlignment horizontal);

  // Sets |entity|'s |vertical| alignment. Takes effect on the next call to
  // SetText.  Bug to fix: TODO(b/37951300)
  void SetVerticalAlignment(Entity entity, VerticalAlignment vertical);

  // Sets text |direction| to Right to left/Left to right mode.
  void SetTextDirection(TextDirection direction);

  // Returns a vector of tags associated with |entity|. This will only be
  // populated if parse_and_strip_html is set to true in the TextDef. Returns
  // null if |entity| does not have a TextDef.
  const std::vector<LinkTag>* GetLinkTags(Entity entity) const;

  // Gets all possible caret positions (locations between glyphs where text
  // may be inserted) for a given text entity.  Returns nullptr if |entity| is
  // not text or doesn't support caret.
  const std::vector<mathfu::vec3>* GetCaretPositions(Entity entity) const;

  // Returns true if all text for this entity has loaded.
  bool IsTextReady(Entity entity) const;

  // Updates the worker threads.  Call once per frame before any draw calls.
  // Make sure all uniforms for rendering are updated before this.
  void ProcessTasks();

  // Blocks until all pending operations are complete.  Some implementations
  // may perform glyph mesh or texture generation on background threads, which
  // are normally polled during ProcessTasks.  WaitForAllTasks blocks the
  // calling thread until all background thread tasks have completed.
  void WaitForAllTasks();

  // Returns the implementation, which may be used to access backend-specific
  // API.
  TextSystemImpl* GetImpl();

  static std::string GetUnprocessedText(const std::string& text);

 private:
  std::unique_ptr<TextSystemImpl> impl_;
};

// Interface for text system implementations.  API mirrors the public API of
// TextSystem.
class TextSystemImpl : public System {
 public:
  explicit TextSystemImpl(Registry* registry) : System(registry) {}

  virtual void CreateEmpty(Entity entity) = 0;

  virtual void CreateFromRenderDef(Entity entity,
                                   const RenderDef& render_def) = 0;

  virtual FontPtr LoadFonts(const std::vector<std::string>& names) = 0;

  FontPtr LoadFonts(
      const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>*
          names);

  virtual void SetFont(Entity entity, FontPtr font) = 0;

  virtual const std::string* GetText(Entity entity) const = 0;

  virtual void SetText(Entity entity, const std::string& text) = 0;

  virtual void SetFontSize(Entity entity, float size) = 0;

  virtual void SetBounds(Entity entity, const mathfu::vec2& bounds) = 0;

  virtual void SetWrapMode(Entity entity, TextWrapMode wrap_mode) = 0;

  virtual void SetEllipsis(Entity entity, const std::string& ellipsis) = 0;

  virtual void SetHorizontalAlignment(Entity entity,
                                      HorizontalAlignment horizontal) = 0;

  virtual void SetVerticalAlignment(Entity entity,
                                    VerticalAlignment vertical) = 0;

  virtual void SetTextDirection(TextDirection direction) = 0;

  virtual const std::vector<LinkTag>* GetLinkTags(Entity entity) const = 0;

  virtual const std::vector<mathfu::vec3>* GetCaretPositions(
      Entity entity) const = 0;

  virtual bool IsTextReady(Entity entity) const = 0;

  virtual void ProcessTasks() = 0;

  virtual void WaitForAllTasks() = 0;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::TextSystemPreprocessingModes);
LULLABY_SETUP_TYPEID(lull::TextSystem);

#endif  // LULLABY_SYSTEMS_TEXT_TEXT_SYSTEM_H_
