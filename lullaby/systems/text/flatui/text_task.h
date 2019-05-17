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

#ifndef LULLABY_SYSTEMS_TEXT_FLATUI_TEXT_TASK_H_
#define LULLABY_SYSTEMS_TEXT_FLATUI_TEXT_TASK_H_

#include "lullaby/util/entity.h"
#include "lullaby/systems/text/flatui/font.h"
#include "lullaby/systems/text/flatui/text_buffer.h"
#include "lullaby/util/async_processor.h"

namespace lull {

// Task to generate a text buffer.
class TextTask {
 public:
  TextTask(Entity target_entity, Entity desired_size_source,
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

using TextTaskPtr = std::shared_ptr<TextTask>;
using TextTaskQueue = AsyncProcessor<TextTaskPtr>;

}  // namespace lull

#endif  // LULLABY_SYSTEMS_TEXT_FLATUI_TEXT_TASK_H_
