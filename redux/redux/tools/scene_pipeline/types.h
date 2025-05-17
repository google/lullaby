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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_TYPES_H_
#define REDUX_TOOLS_SCENE_PIPELINE_TYPES_H_

// Macro to define a constant string just to aid in readability.
#define SCENE_NAME(A, B) inline static constexpr std::string_view A = B

namespace redux::tool {

// Common vector and matrix types used to describe scenes. While these types
// are mathematical in nature, we do not implement math operations for them.

struct int2 {
  constexpr int2() = default;
  constexpr int2(int x, int y) : x(x), y(y) {}
  explicit int2(const int* ptr) : x(ptr[0]), y(ptr[1]) {}

  int x = 0;
  int y = 0;
};

struct int3 {
  constexpr int3() = default;
  constexpr int3(int x, int y, int z) : x(x), y(y), z(z) {}
  explicit int3(const int* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]) {}

  int x = 0;
  int y = 0;
  int z = 0;
};

struct int4 {
  constexpr int4() = default;
  constexpr int4(int x, int y, int z, int w) : x(x), y(y), z(z), w(w) {}
  explicit int4(const int* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]), w(ptr[3]) {}

  int x = 0;
  int y = 0;
  int z = 0;
  int w = 0;
};

struct float2 {
  constexpr float2() = default;
  constexpr float2(float x, float y) : x(x), y(y) {}
  explicit float2(const float* ptr) : x(ptr[0]), y(ptr[1]) {}

  float x = 0.f;
  float y = 0.f;
};

struct float3 {
  constexpr float3() = default;
  constexpr float3(float x, float y, float z) : x(x), y(y), z(z) {}
  explicit float3(const float* ptr) : x(ptr[0]), y(ptr[1]), z(ptr[2]) {}

  float x = 0.f;
  float y = 0.f;
  float z = 0.f;
};

struct float4 {
  constexpr float4() = default;
  constexpr float4(float x, float y, float z, float w)
      : x(x), y(y), z(z), w(w) {}
  explicit float4(const float* ptr)
      : x(ptr[0]), y(ptr[1]), z(ptr[2]), w(ptr[3]) {}

  float x = 0.f;
  float y = 0.f;
  float z = 0.f;
  float w = 0.f;
};

struct float4x4 {
  constexpr float4x4() {
    // Initializes to an identity matrix.
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        if (i == j) {
          data[i][j] = 1.f;
        } else {
          data[i][j] = 0.f;
        }
      }
    }
  }

  explicit float4x4(const float* ptr) {
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        data[i][j] = ptr[i * 4 + j];
      }
    }
  }

  float data[4][4];
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_TYPES_H_
