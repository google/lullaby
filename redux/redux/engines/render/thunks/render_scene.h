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

#ifndef REDUX_ENGINES_RENDER_THUNKS_RENDER_SCENE_H_
#define REDUX_ENGINES_RENDER_THUNKS_RENDER_SCENE_H_

#include "redux/engines/render/render_scene.h"

namespace redux {

// Thunk functions to call the actual implementation.
void RenderScene::Add(const Renderable& renderable) {
  Upcast(this)->Add(renderable);
}
void RenderScene::Remove(const Renderable& renderable) {
  Upcast(this)->Remove(renderable);
}
void RenderScene::Add(const Light& light) { Upcast(this)->Add(light); }
void RenderScene::Remove(const Light& light) { Upcast(this)->Remove(light); }
void RenderScene::Add(const IndirectLight& light) { Upcast(this)->Add(light); }
void RenderScene::Remove(const IndirectLight& light) {
  Upcast(this)->Remove(light);
}

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_THUNKS_RENDER_SCENE_H_
