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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_GL_HELPERS_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_GL_HELPERS_H_

#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/sanitize_shader_source.h"
#include "lullaby/modules/render/vertex_format.h"
#include "lullaby/systems/render/next/detail/glplatform.h"
#include "lullaby/util/span.h"
#include "lullaby/generated/render_target_def_generated.h"
#include "lullaby/generated/texture_def_generated.h"

namespace lull {

GLenum GetGlInternalFormat(DepthStencilFormat format);
GLenum GetGlInternalFormat(TextureFormat format);
GLenum GetGlFormat(TextureFormat format);
GLenum GetGlType(TextureFormat format);
GLenum GetGlTextureFiltering(TextureFiltering filtering);
GLenum GetGlTextureWrap(TextureWrap wrap);
/// Returns the GLenum mode for drawing based on the primitive type.
GLenum GetGlPrimitiveType(MeshData::PrimitiveType type);
/// Returns the GLenum corresponding with the vertex type.
GLenum GetGlVertexType(VertexAttributeType type);
/// Returns the number of data elements in the given vertex data type.
GLint GetNumElementsInVertexType(VertexAttributeType type);
/// Returns the GLenum data type based on the index type.
GLenum GetGlIndexType(MeshData::IndexType type);

ShaderProfile GetShaderProfile();

int GlMaxVertexUniformComponents();
bool GlSupportsVertexArrays();
bool GlSupportsTextureNpot();
bool GlSupportsAstc();
bool GlSupportsEtc2();

/// Sets the gl vertex attributes.
void SetVertexAttributes(const VertexFormat& vertex_format,
                         const uint8_t* buffer = nullptr);
/// Unsets a gl vertex format.
void UnsetVertexAttributes(const VertexFormat& vertex_format);

/// Draws mesh data.
void DrawMeshData(const MeshData& mesh_data);
/// Draws a quad.
void DrawQuad(const mathfu::vec3& bottom_left, const mathfu::vec3& top_right,
              const mathfu::vec2& tex_bottom_left,
              const mathfu::vec2& tex_top_right);

// Returns the default vertex attributes used by Lullaby.
Span<std::pair<const char*, int>> GetDefaultVertexAttributes();

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_GL_HELPERS_H_
