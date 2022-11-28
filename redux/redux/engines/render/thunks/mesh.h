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

#ifndef REDUX_ENGINES_RENDER_THUNKS_MESH_H_
#define REDUX_ENGINES_RENDER_THUNKS_MESH_H_

#include "redux/engines/render/renderable.h"

namespace redux {

// Thunk functions to call the actual implementation.
size_t Mesh::GetNumVertices() const { return Upcast(this)->GetNumVertices(); }
size_t Mesh::GetNumPrimitives() const {
  return Upcast(this)->GetNumPrimitives();
}
Box Mesh::GetBoundingBox() const { return Upcast(this)->GetBoundingBox(); }
size_t Mesh::GetNumSubmeshes() const { return Upcast(this)->GetNumSubmeshes(); }
const Mesh::SubmeshData& Mesh::GetSubmeshData(size_t index) const {
  return Upcast(this)->GetSubmeshData(index);
}

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_THUNKS_MESH_H_
