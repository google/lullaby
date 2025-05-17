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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_SCENE_H_
#define REDUX_TOOLS_SCENE_PIPELINE_SCENE_H_

#include "redux/tools/scene_pipeline/buffer.h"
#include "redux/tools/scene_pipeline/buffer_view.h"
#include "redux/tools/scene_pipeline/collider.h"
#include "redux/tools/scene_pipeline/drawable.h"
#include "redux/tools/scene_pipeline/image.h"
#include "redux/tools/scene_pipeline/material.h"
#include "redux/tools/scene_pipeline/model.h"
#include "redux/tools/scene_pipeline/safe_vector.h"

namespace redux::tool {

// The collection of data that represents a Scene.
//
// A Scene primarily consists of a collection of Models. Each Model represents
// a single object in the scene, e.g. a chair or a table. Each Model, in turn,
// may have references to the other data contained in the Scene. This allows
// for a more efficient storage (e.g. two chairs in the Scene may share the
// same geometry).
//
// All the data in the Scene is owned by the Scene. This allows users to
// std::move the data as needed, e,g. passing it into physics or rendering
// engines.
struct Scene {
  // The models (objects) that make up the scene.
  SafeVector<Model> models;

  // All data buffers that were created during scene construction. For example,
  // geometry (vertex) data, pixel data for images, etc. Other objects in the
  // scene reference these buffers using BufferViews.
  SafeVector<Buffer> buffers;

  // All drawables used in the scene. These are the actual objects that intended
  // for rendering and are referenced by the Models via a DrawableIndex.
  SafeVector<Drawable> drawables;

  // All colliders used in the scene. These are the actual objects that intended
  // for use by physics and are referenced by the Models via a ColliderIndex.
  SafeVector<Collider> colliders;

  // All materials used in the scene. Drawables can refer to these materials via
  // a MaterialIndex.
  SafeVector<Material> materials;

  // All images used in the scene. Materials may refer to these images
  // (textures) via an ImageIndex.
  SafeVector<Image> images;

  // Resolves a BufferView into a span of bytes.
  ByteSpan span(BufferView view) const {
    const Buffer& buffer = buffers[view.buffer_index];
    return buffer.subspan(view.offset, view.length);
  }

  // Traverses the data in a BufferView, treating it as a sequence of values of
  // the given type. The stride is the number of bytes between each value.
  template <typename DataType, typename CallbackFn>
  void Traverse(BufferView view, int stride, CallbackFn&& cb) const {
    const ByteSpan span = this->span(view);
    for (int i = 0; i < span.size(); i += stride) {
      cb(*reinterpret_cast<const DataType*>(&span[i]));
    }
  }
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_SCENE_H_
