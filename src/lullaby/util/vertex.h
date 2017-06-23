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

#ifndef LULLABY_UTIL_VERTEX_H_
#define LULLABY_UTIL_VERTEX_H_

#include "mathfu/glsl_mappings.h"
#include "lullaby/util/color.h"
#include "lullaby/util/vertex_format.h"

namespace lull {

// Below are several common vertex structures and utilities to access their
// properties.  All vertex structures that are going to be used for dynamic
// rendering must include a static VertexFormat kFormat.

struct VertexP {
  static const VertexFormat kFormat;

  VertexP() {}
  VertexP(float px, float py, float pz) : x(px), y(py), z(pz) {}
  explicit VertexP(const mathfu::vec3& pos) : x(pos.x), y(pos.y), z(pos.z) {}

  float x;
  float y;
  float z;
};

struct VertexPT {
  static const VertexFormat kFormat;

  VertexPT() {}
  VertexPT(float px, float py, float pz, float u, float v)
      : x(px), y(py), z(pz), u0(u), v0(v) {}
  VertexPT(const mathfu::vec3& pos, const mathfu::vec2& uv)
      : x(pos.x), y(pos.y), z(pos.z), u0(uv.x), v0(uv.y) {}

  float x;
  float y;
  float z;
  float u0;
  float v0;
};

struct VertexPTT {
  static const VertexFormat kFormat;

  VertexPTT() {}
  VertexPTT(float px, float py, float pz, float u0, float v0, float u1,
            float v1)
      : x(px), y(py), z(pz), u0(u0), v0(v0), u1(u1), v1(v1) {}
  VertexPTT(const mathfu::vec3& pos, const mathfu::vec2& uv0,
            const mathfu::vec2& uv1)
      : x(pos.x),
        y(pos.y),
        z(pos.z),
        u0(uv0.x),
        v0(uv0.y),
        u1(uv1.x),
        v1(uv1.y) {}

  float x;
  float y;
  float z;
  float u0;
  float v0;
  float u1;
  float v1;
};

struct VertexPN {
  static const VertexFormat kFormat;

  VertexPN() {}
  VertexPN(float px, float py, float pz, float pnx, float pny, float pnz)
      : x(px), y(py), z(pz), nx(pnx), ny(pny), nz(pnz) {}
  VertexPN(const mathfu::vec3& pos, const mathfu::vec3& n)
      : x(pos.x), y(pos.y), z(pos.z), nx(n.x), ny(n.y), nz(n.z) {}

  float x;
  float y;
  float z;
  float nx;
  float ny;
  float nz;
};

struct VertexPC {
  static const VertexFormat kFormat;

  VertexPC() {}
  VertexPC(float px, float py, float pz, Color4ub color)
      : x(px), y(py), z(pz), color(color) {}
  VertexPC(const mathfu::vec3& pos, Color4ub color)
      : x(pos.x), y(pos.y), z(pos.z), color(color) {}

  float x;
  float y;
  float z;
  Color4ub color;
};

struct VertexPTC {
  static const VertexFormat kFormat;

  VertexPTC() {}
  VertexPTC(float px, float py, float pz, float u, float v, Color4ub color)
      : x(px), y(py), z(pz), u0(u), v0(v), color(color) {}
  VertexPTC(const mathfu::vec3& pos, const mathfu::vec2& uv, Color4ub color)
      : x(pos.x), y(pos.y), z(pos.z), u0(uv.x), v0(uv.y), color(color) {}

  float x;
  float y;
  float z;
  float u0;
  float v0;
  Color4ub color;
};

struct VertexPTN {
  static const VertexFormat kFormat;

  VertexPTN() {}
  VertexPTN(float px, float py, float pz, float u, float v,
            float pnx, float pny, float pnz)
      : x(px), y(py), z(pz), u0(u), v0(v), nx(pnx), ny(pny), nz(pnz) {}
  VertexPTN(const mathfu::vec3& pos, const mathfu::vec2& uv,
            const mathfu::vec3& n)
      : x(pos.x),
        y(pos.y),
        z(pos.z),
        u0(uv.x),
        v0(uv.y),
        nx(n.x),
        ny(n.y),
        nz(n.z) {}

  float x;
  float y;
  float z;
  float u0;
  float v0;
  float nx;
  float ny;
  float nz;
};

struct VertexPTI {
  static const VertexFormat kFormat;

  VertexPTI() {}
  VertexPTI(float px, float py, float pz, float u, float v,
            const uint8_t indices[4])
      : x(px), y(py), z(pz), u0(u), v0(v) {
    for (int i = 0; i < 4; ++i) {
      this->indices[i] = indices[i];
    }
  }
  VertexPTI(const mathfu::vec3& pos, const mathfu::vec2& uv,
            const uint8_t indices[4])
      : x(pos.x), y(pos.y), z(pos.z), u0(uv.x), v0(uv.y) {
    for (int i = 0; i < 4; ++i) {
      this->indices[i] = indices[i];
    }
  }

  float x;
  float y;
  float z;
  float u0;
  float v0;
  uint8_t indices[4];
};

struct VertexPTTI {
  static const VertexFormat kFormat;

  VertexPTTI() {}
  VertexPTTI(float px, float py, float pz, float u0, float v0, float u1,
             float v1, const uint8_t indices[4])
      : x(px), y(py), z(pz), u0(u0), v0(v0), u1(u1), v1(v1) {
    for (int i = 0; i < 4; ++i) {
      this->indices[i] = indices[i];
    }
  }
  VertexPTTI(const mathfu::vec3& pos, const mathfu::vec2& uv0,
             const mathfu::vec2& uv1, const uint8_t indices[4])
      : x(pos.x),
        y(pos.y),
        z(pos.z),
        u0(uv0.x),
        v0(uv0.y),
        u1(uv1.x),
        v1(uv1.y) {
    for (int i = 0; i < 4; ++i) {
      this->indices[i] = indices[i];
    }
  }

  float x;
  float y;
  float z;
  float u0;
  float v0;
  float u1;
  float v1;
  uint8_t indices[4];
};

template <typename Vertex>
inline mathfu::vec3 GetPosition(const Vertex& v) {
  return mathfu::vec3(v.x, v.y, v.z);
}

template <typename Vertex>
inline void SetPosition(Vertex* vertex, float x, float y, float z) {
  vertex->x = x;
  vertex->y = y;
  vertex->z = z;
}

template <typename Vertex>
inline void SetPosition(Vertex* vertex, const mathfu::vec3& pos) {
  vertex->x = pos.x;
  vertex->y = pos.y;
  vertex->z = pos.z;
}

template <typename Vertex>
inline mathfu::vec2 GetUv0(const Vertex& v) {
  return mathfu::vec2(v.u0, v.v0);
}

template <typename Vertex>
inline mathfu::vec2 GetUv1(const Vertex& v) {
  return mathfu::vec2(v.u1, v.v1);
}

template <typename Vertex>
inline void SetUv0(Vertex* vertex, float u, float v) {
  vertex->u0 = u;
  vertex->v0 = v;
}

template <typename Vertex>
inline void SetUv1(Vertex* vertex, float u, float v) {}

template <>
inline void SetUv1<VertexPTT>(VertexPTT* vertex, float u, float v) {
  vertex->u1 = u;
  vertex->v1 = v;
}

template <>
inline void SetUv0<VertexP>(VertexP* vertex, float u, float v) {}

template <typename Vertex>
inline void SetUv0(Vertex* vertex, const mathfu::vec2& uv) {
  vertex->u0 = uv.x;
  vertex->v0 = uv.y;
}

template <typename Vertex>
inline void SetUv1(Vertex* vertex, const mathfu::vec2& uv) {}

template <>
inline void SetUv1<VertexPTT>(VertexPTT* vertex, const mathfu::vec2& uv) {
  vertex->u1 = uv.x;
  vertex->v1 = uv.y;
}

template <>
inline void SetUv1<VertexPTTI>(VertexPTTI* vertex, const mathfu::vec2& uv) {
  vertex->u1 = uv.x;
  vertex->v1 = uv.y;
}

template <>
inline void SetUv0<VertexP>(VertexP* vertex, const mathfu::vec2& uv) {}

template <>
inline void SetUv0<VertexPC>(VertexPC* vertex, const mathfu::vec2& uv) {}

template <typename Vertex>
inline void SetColor(Vertex* vertex, Color4ub color) {
  vertex->color = color;
}

template <typename Vertex>
inline mathfu::vec3 GetNormal(const Vertex& v) {
  return mathfu::vec3(v.nx, v.ny, v.nz);
}

template <typename Vertex>
inline void SetNormal(Vertex* vertex, float nx, float ny, float nz) {
  vertex->nx = nx;
  vertex->ny = ny;
  vertex->nz = nz;
}

template <typename Vertex>
inline void SetNormal(Vertex* vertex, const mathfu::vec3& n) {
  vertex->nx = n.x;
  vertex->ny = n.y;
  vertex->nz = n.z;
}

template <>
inline void SetColor<VertexP>(VertexP* vertex, Color4ub color) {}

template <>
inline void SetColor<VertexPT>(VertexPT* vertex, Color4ub color) {}

template <>
inline void SetColor<VertexPTTI>(VertexPTTI* vertex, Color4ub color) {}

template <>
inline void SetColor<VertexPN>(VertexPN* vertex, Color4ub color) {}

template <>
inline void SetColor<VertexPTN>(VertexPTN* vertex, Color4ub color) {}

// Calls |fn| for each vertex position in |vertex_data|.  |fn| should match the
// prototype void()(const vec3&).  This vec3 is a copy of, not a reference to,
// the stored position data.
template <typename Functor>
void ForEachVertexPosition(const uint8_t* vertex_data, size_t vertex_count,
                           const VertexFormat& format, const Functor& fn) {
  if (format.GetAttributeAt(0).usage != VertexAttribute::kPosition) {
    LOG(DFATAL) << "Vertex format missing position attribute";
    return;
  }

  for (size_t index = 0; index < vertex_count; ++index) {
    const uint8_t* vertex = vertex_data + (index * format.GetVertexSize());
    mathfu::vec3 position;
    memcpy(&position, vertex, sizeof(position));
    fn(position);
  }
}

}  // namespace lull

#endif  // LULLABY_UTIL_VERTEX_H_
