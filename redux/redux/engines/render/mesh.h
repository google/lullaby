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

#ifndef REDUX_ENGINES_RENDER_MESH_H_
#define REDUX_ENGINES_RENDER_MESH_H_

#include <cstddef>
#include <memory>
#include "redux/modules/base/hash.h"

namespace redux {

// Geometry (eg. vertices, indices, primitive type) used in a draw call.
//
// A mesh can be composed of multiple submeshes, each of which can be drawn
// independently of each other.
class Mesh {
 public:
  virtual ~Mesh() = default;

  // Returns the number of parts in the mesh.
  size_t GetNumParts() const;

  // Returns the name of the part at the given index.
  HashValue GetPartName(size_t index) const;

  Mesh(const Mesh&) = delete;
  Mesh& operator=(const Mesh&) = delete;

 protected:
  Mesh() = default;
};

using MeshPtr = std::shared_ptr<Mesh>;

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_MESH_H_
