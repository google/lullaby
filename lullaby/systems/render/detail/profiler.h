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

#ifndef LULLABY_SYSTEMS_RENDER_DETAIL_PROFILER_H_
#define LULLABY_SYSTEMS_RENDER_DETAIL_PROFILER_H_

#include "lullaby/systems/render/detail/gpu_profiler.h"
#include "lullaby/systems/render/shader.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/typeid.h"

namespace lull {
namespace detail {

class Profiler {
 public:
  Profiler();

  // Returns the frame rate filtered over the last several frames.
  float GetFilteredFps() const;

  // Returns the frame rate of the last available frame.
  float GetLastFps() const;

  // Returns the CPU time in milliseconds of the last available frame.
  float GetCpuFrameMs() const;

  // Returns the GPU time in milliseconds of the last available frame, or 0.0
  // if GPU timing is not supported.
  float GetGpuFrameMs() const;

  // Returns the number of draw calls during the last available frame.
  int GetNumDraws() const;

  // Returns the number of shader swaps during the last available frame.
  int GetNumShaderSwaps() const;

  // Returns the number of verts used during the last available frame.
  int GetNumVerts() const;

  // Returns the number of tris used during the last available frame.
  int GetNumTris() const;

  // Returns an estimated number of dropped frames.
  int GetNumDroppedFrames() const { return num_dropped_frames_; }

  // Resets the dropped frame counter to 0.
  void ResetNumDroppedFrames() { num_dropped_frames_ = 0; }

  // Marks the beginning of a frame.
  void BeginFrame();

  // Marks the end of a frame.
  void EndFrame();

  // Records a draw call using |shader| with |num_verts| and |num_tris|.
  void RecordDraw(ShaderPtr shader, int num_verts,  int num_tris);

 private:
  static const int kMaxFrames = 10;

  struct Marker {
    Clock::time_point cpu;
    GpuProfiler::Query gpu_marker = GpuProfiler::kInvalidQuery;
    uint64_t gpu_time_nanosec = 0;
  };

  struct Frame {
    Marker begin;
    Marker end;
    float cpu_duration_ms = 0;
    float gpu_duration_ms = 0;
    float cpu_interval_ms = 0;
    float gpu_interval_ms = 0;

    ShaderPtr last_shader;
    int num_draws = 0;
    int num_shader_swaps = 0;
    int num_verts = 0;
    int num_tris = 0;
  };

  Marker SetMarker();
  void PollMarker(Marker* m);
  void ResetMarker(Marker* m);

  bool IsFrameProfiled(const Frame& f) const;
  float GetFrameFps(const Frame& f) const;
  const Frame* GetMostRecentProfiledFrame() const;
  void ResetFrame(Frame* f);

  GpuProfiler gpu_;
  Frame frames_[kMaxFrames];
  int head_ = 0;
  bool in_frame_ = false;
  int num_dropped_frames_ = 0;
};

}  // namespace detail
}  // namespace lull

LULLABY_SETUP_TYPEID(lull::detail::Profiler);

#endif  // LULLABY_SYSTEMS_RENDER_DETAIL_PROFILER_H_
