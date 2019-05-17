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

#include "lullaby/systems/text/flatui/text_task.h"

namespace lull {

TextTask::TextTask(
    Entity target_entity, Entity desired_size_source, const FontPtr& font,
    const std::string& text, const TextBufferParams& params)
    : target_entity_(target_entity),
      desired_size_source_(desired_size_source),
      font_(font),
      text_(text),
      params_(params),
      text_buffer_(nullptr),
      output_text_buffer_(nullptr) {}

void TextTask::Process() {
  if (font_ && font_->Bind()) {
    text_buffer_ = TextBuffer::Create(font_->GetFontManager(), text_, params_);
  } else {
    LOG(ERROR) << "Font is null or failed to bind in "
                  "GenerateTextBufferTask::Process()";
  }
}

void TextTask::Finalize() {
  if (text_buffer_) {
    // TODO Remove Finalize.
    text_buffer_->Finalize();
    using std::swap;
    swap(text_buffer_, output_text_buffer_);
  }
}

}  // namespace lull
