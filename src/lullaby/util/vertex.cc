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

#include "lullaby/util/vertex.h"

namespace lull {

// The standard position attribute expected in every vertex format instance.
static VertexAttribute PositionAttribute() {
  return VertexAttribute(0, VertexAttribute::kPosition, 3,
                         VertexAttribute::kFloat32);
}

const VertexFormat VertexP::kFormat({PositionAttribute()});

const VertexFormat VertexPT::kFormat({
    PositionAttribute(),
    VertexAttribute(12, VertexAttribute::kTexCoord, 2,
                    VertexAttribute::kFloat32),
});

const VertexFormat VertexPTT::kFormat({
    PositionAttribute(),
    VertexAttribute(12, VertexAttribute::kTexCoord, 2,
                    VertexAttribute::kFloat32),
    VertexAttribute(20, VertexAttribute::kTexCoord, 2,
                    VertexAttribute::kFloat32, 1),
});

const VertexFormat VertexPN::kFormat({
    PositionAttribute(),
    VertexAttribute(12, VertexAttribute::kNormal, 3, VertexAttribute::kFloat32),
});

const VertexFormat VertexPC::kFormat({
    PositionAttribute(),
    VertexAttribute(12, VertexAttribute::kColor, 4,
                    VertexAttribute::kUnsignedInt8),
});

const VertexFormat VertexPTC::kFormat({
    PositionAttribute(),
    VertexAttribute(12, VertexAttribute::kTexCoord, 2,
                    VertexAttribute::kFloat32),
    VertexAttribute(20, VertexAttribute::kColor, 4,
                    VertexAttribute::kUnsignedInt8),
});

const VertexFormat VertexPTN::kFormat({
    PositionAttribute(),
    VertexAttribute(12, VertexAttribute::kTexCoord, 2,
                    VertexAttribute::kFloat32),
    VertexAttribute(20, VertexAttribute::kNormal, 3, VertexAttribute::kFloat32),
});

const VertexFormat VertexPTI::kFormat({
    PositionAttribute(),
    VertexAttribute(12, VertexAttribute::kTexCoord, 2,
                    VertexAttribute::kFloat32),
    VertexAttribute(20, VertexAttribute::kIndex, 4,
                    VertexAttribute::kUnsignedInt8),
});

const VertexFormat VertexPTTI::kFormat({
    VertexAttribute(0, VertexAttribute::kPosition, 3,
                    VertexAttribute::kFloat32),
    VertexAttribute(12, VertexAttribute::kTexCoord, 2,
                    VertexAttribute::kFloat32),
    VertexAttribute(20, VertexAttribute::kTexCoord, 2,
                    VertexAttribute::kFloat32, 1),
    VertexAttribute(28, VertexAttribute::kIndex, 4,
                    VertexAttribute::kUnsignedInt8),
});

}  // namespace lull
