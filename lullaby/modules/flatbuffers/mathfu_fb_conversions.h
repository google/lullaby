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

#ifndef LULLABY_MODULES_FLATBUFFERS_MATHFU_FB_CONVERSIONS_H_
#define LULLABY_MODULES_FLATBUFFERS_MATHFU_FB_CONVERSIONS_H_

#include "mathfu/matrix.h"
#include "mathfu/vector.h"
#include "lullaby/generated/common_generated.h"
#include "lullaby/util/arc.h"
#include "lullaby/util/color.h"
#include "lullaby/util/math.h"

namespace lull {

// ---------------------------------------------------------------------------
// Functions to convert between mathfu and flatbuffer common types.
// ---------------------------------------------------------------------------

inline void MathfuVec2FromFbVec2(const Vec2* in, mathfu::vec2* out) {
  if (in && out) {
    *out = mathfu::vec2(in->x(), in->y());
  }
}

inline void MathfuVec2iFromFbVec2i(const Vec2i* in, mathfu::vec2i* out) {
  if (in && out) {
    *out = mathfu::vec2i(in->x(), in->y());
  }
}

inline void MathfuVec3FromFbVec3(const Vec3* in, mathfu::vec3* out) {
  if (in && out) {
    *out = mathfu::vec3(in->x(), in->y(), in->z());
  }
}

inline void MathfuVec4FromFbVec4(const Vec4* in, mathfu::vec4* out) {
  if (in && out) {
    *out = mathfu::vec4(in->x(), in->y(), in->z(), in->w());
  }
}

inline void MathfuQuatFromFbQuat(const Quat* in, mathfu::quat* out) {
  if (in && out) {
    *out = mathfu::quat(in->w(), in->x(), in->y(), in->z()).Normalized();
  }
}

inline void MathfuQuatFromFbVec3(const Vec3* in, mathfu::quat* out) {
  if (in && out) {
    const mathfu::vec3 angles(in->x(), in->y(), in->z());
    *out = mathfu::quat::FromEulerAngles(angles * kDegreesToRadians);
  }
}

inline void MathfuQuatFromFbVec4(const Vec4* in, mathfu::quat* out) {
  if (in && out) {
    *out = mathfu::quat(in->w(), in->x(), in->y(), in->z());
  }
}

inline void MathfuVec4FromFbColor(const Color* in, mathfu::vec4* out) {
  if (in && out) {
    *out = mathfu::vec4(in->r(), in->g(), in->b(), in->a());
  }
}

inline void MathfuVec4FromFbColorHex(const char* in, mathfu::vec4* out) {
  if (in && out) {
    if (*in == '#') {
      ++in;
    }

    const size_t len = strlen(in);
    if (len != 6 && len != 8) {
      return;
    }

    *out = mathfu::vec4(0.f, 0.f, 0.f, 1.f);
    for (int i = 0; i < 4; ++i) {
      if (*in == 0) {
        break;
      }

      const char hexstr[3] = {in[0], in[1], 0};
      in += 2;

      char* end;
      const auto value =
          std::strtoul(hexstr, &end, 16);
      (*out)[i] = static_cast<float>(value) / 255.f;
    }
  }
}

inline void AabbFromFbAabb(const AabbDef* in, Aabb* out) {
  if (in && out) {
    MathfuVec3FromFbVec3(&in->min(), &out->min);
    MathfuVec3FromFbVec3(&in->max(), &out->max);
  }
}

inline void AabbFromFbRect(const Rect* in, Aabb* out) {
  if (in && out) {
    out->min = mathfu::vec3(in->x(), in->y(), 0.0f);
    out->max = out->min + mathfu::vec3(in->w(), in->h(), 0.0f);
  }
}

inline void ArcFromFbArcDef(const ArcDef* in, Arc* out) {
  if (in && out) {
    out->start_angle = in->start_angle();
    out->angle_size = in->angle_size();
    out->inner_radius = in->inner_radius();
    out->outer_radius = in->outer_radius();
    out->num_samples = in->num_samples();
  }
}

inline void Color4ubFromFbColor(const Color* in, Color4ub* out) {
  if (in && out) {
    mathfu::vec4 v;
    MathfuVec4FromFbColor(in, &v);
    *out = Color4ub(v);
  }
}

}  // namespace lull

#endif  // LULLABY_MODULES_FLATBUFFERS_MATHFU_FB_CONVERSIONS_H_
