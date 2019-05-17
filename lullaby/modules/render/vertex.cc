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

#include "lullaby/modules/render/vertex.h"

namespace lull {

const VertexFormat VertexP::kFormat({{VertexAttributeUsage_Position,
                                      VertexAttributeType_Vec3f}});

const VertexFormat VertexPT::kFormat({
    {VertexAttributeUsage_Position, VertexAttributeType_Vec3f},
    {VertexAttributeUsage_TexCoord, VertexAttributeType_Vec2f},
});

const VertexFormat VertexPTT::kFormat({
    {VertexAttributeUsage_Position, VertexAttributeType_Vec3f},
    {VertexAttributeUsage_TexCoord, VertexAttributeType_Vec2f},
    {VertexAttributeUsage_TexCoord, VertexAttributeType_Vec2f},
});

const VertexFormat VertexPTTN::kFormat({
    {VertexAttributeUsage_Position, VertexAttributeType_Vec3f},
    {VertexAttributeUsage_TexCoord, VertexAttributeType_Vec2f},
    {VertexAttributeUsage_TexCoord, VertexAttributeType_Vec2f},
    {VertexAttributeUsage_Normal, VertexAttributeType_Vec3f},
});

const VertexFormat VertexPN::kFormat({
    {VertexAttributeUsage_Position, VertexAttributeType_Vec3f},
    {VertexAttributeUsage_Normal, VertexAttributeType_Vec3f},
});

const VertexFormat VertexPC::kFormat({
    {VertexAttributeUsage_Position, VertexAttributeType_Vec3f},
    {VertexAttributeUsage_Color, VertexAttributeType_Vec4ub},
});

const VertexFormat VertexPTC::kFormat({
    {VertexAttributeUsage_Position, VertexAttributeType_Vec3f},
    {VertexAttributeUsage_TexCoord, VertexAttributeType_Vec2f},
    {VertexAttributeUsage_Color, VertexAttributeType_Vec4ub},
});

const VertexFormat VertexPTN::kFormat({
    {VertexAttributeUsage_Position, VertexAttributeType_Vec3f},
    {VertexAttributeUsage_TexCoord, VertexAttributeType_Vec2f},
    {VertexAttributeUsage_Normal, VertexAttributeType_Vec3f},
});

const VertexFormat VertexPTI::kFormat({
    {VertexAttributeUsage_Position, VertexAttributeType_Vec3f},
    {VertexAttributeUsage_TexCoord, VertexAttributeType_Vec2f},
    {VertexAttributeUsage_BoneIndices, VertexAttributeType_Vec4ub},
});

const VertexFormat VertexPTTI::kFormat({
    {VertexAttributeUsage_Position, VertexAttributeType_Vec3f},
    {VertexAttributeUsage_TexCoord, VertexAttributeType_Vec2f},
    {VertexAttributeUsage_TexCoord, VertexAttributeType_Vec2f},
    {VertexAttributeUsage_BoneIndices, VertexAttributeType_Vec4ub},
});

}  // namespace lull
