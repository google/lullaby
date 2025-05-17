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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_INDEX_BUFFER_H_
#define REDUX_TOOLS_SCENE_PIPELINE_INDEX_BUFFER_H_

#include "redux/tools/scene_pipeline/buffer_view.h"
#include "redux/tools/scene_pipeline/type_id.h"

namespace redux::tool {

// A view to a buffer of indices.
struct IndexBuffer {
  // The type (e.g. uint16_t or uint32_t) of each element in the buffer.
  TypeId type;

  // The buffer containing the indices.
  BufferView buffer_view;

  // The number of indices in the buffer. (i.e. buffer.size() / sizeof[type]).
  int num_indices = 0;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_INDEX_BUFFER_H_
