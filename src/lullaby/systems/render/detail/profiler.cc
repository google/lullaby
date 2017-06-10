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

#include "lullaby/systems/render/detail/profiler.h"

#include "lullaby/util/logging.h"
#include "lullaby/util/time.h"

namespace lull {
namespace detail {
namespace {
// Couldn't find any hard data on phone refresh rate, so for now will stick with
// the old NTSC standard of 59.94 Hz.
constexpr float kNominalFrameIntervalMS = 1000.0f / 59.94f;
constexpr float kDroppedFrameAllowanceMS = .2f;
}  // namespace

Profiler::Profiler() {}

float Profiler::GetFilteredFps() const {
  float total = 0.0f;
  int count = 0;

  for (int i = 0; i < kMaxFrames; ++i) {
    const Frame& f = frames_[i];
    if (IsFrameProfiled(f)) {
      total += GetFrameFps(f);
      ++count;
    }
  }

  if (count == 0) {
    return 0;
  }
  return total / static_cast<float>(count);
}

float Profiler::GetLastFps() const {
  const Frame* f = GetMostRecentProfiledFrame();
  return (f ? GetFrameFps(*f) : 0.0f);
}

float Profiler::GetCpuFrameMs() const {
  const Frame* f = GetMostRecentProfiledFrame();
  return (f ? f->cpu_duration_ms : 0.0f);
}

float Profiler::GetGpuFrameMs() const {
  const Frame* f = GetMostRecentProfiledFrame();
  return (f ? f->gpu_duration_ms : 0.0f);
}

int Profiler::GetNumDraws() const {
  const Frame* f = GetMostRecentProfiledFrame();
  return (f ? f->num_draws : 0);
}

int Profiler::GetNumShaderSwaps() const {
  const Frame* f = GetMostRecentProfiledFrame();
  return (f ? f->num_shader_swaps : 0);
}

int Profiler::GetNumVerts() const {
  const Frame* f = GetMostRecentProfiledFrame();
  return (f ? f->num_verts : 0);
}

int Profiler::GetNumTris() const {
  const Frame* f = GetMostRecentProfiledFrame();
  return (f ? f->num_tris : 0);
}

Profiler::Marker Profiler::SetMarker() {
  Marker m;
  m.cpu = Clock::now();
  m.gpu_marker = gpu_.SetMarker();
  return m;
}

void Profiler::PollMarker(Marker* m) {
  if (m->gpu_marker != GpuProfiler::kInvalidQuery &&
      gpu_.GetTime(m->gpu_marker, &m->gpu_time_nanosec)) {
    m->gpu_marker = GpuProfiler::kInvalidQuery;
  }
}

void Profiler::ResetMarker(Marker* m) {
  if (m->gpu_marker != GpuProfiler::kInvalidQuery) {
    gpu_.Abandon(m->gpu_marker);
    m->gpu_marker = GpuProfiler::kInvalidQuery;
  }
  m->gpu_time_nanosec = 0;
}

void Profiler::ResetFrame(Frame* f) {
  ResetMarker(&f->begin);
  ResetMarker(&f->end);

  f->cpu_duration_ms = 0.0f;
  f->gpu_duration_ms = 0.0f;
  f->cpu_interval_ms = 0.0f;
  f->gpu_interval_ms = 0.0f;

  f->last_shader.reset();
  f->num_draws = 0;
  f->num_shader_swaps = 0;
  f->num_verts = 0;
  f->num_tris = 0;
}

void Profiler::BeginFrame() {
  CHECK(!in_frame_);

  gpu_.BeginFrame();

  for (int i = 0; i < kMaxFrames; ++i) {
    Frame& f = frames_[i];
    PollMarker(&f.begin);
    PollMarker(&f.end);

    if (f.gpu_duration_ms == 0.0f && f.begin.gpu_time_nanosec != 0 &&
        f.end.gpu_time_nanosec != 0) {
      f.gpu_duration_ms = MillisecondsFromNanoseconds(f.end.gpu_time_nanosec -
                                                      f.begin.gpu_time_nanosec);
    }

    if (f.gpu_interval_ms == 0.0f && f.begin.gpu_time_nanosec != 0) {
      const Frame& prev = frames_[(i + kMaxFrames - 1) % kMaxFrames];
      if (prev.begin.gpu_time_nanosec != 0) {
        f.gpu_interval_ms = MillisecondsFromNanoseconds(
            f.begin.gpu_time_nanosec - prev.begin.gpu_time_nanosec);
      }
    }
  }

  Frame& f = frames_[head_];
  ResetFrame(&f);
  f.begin = SetMarker();
  in_frame_ = true;
}

void Profiler::EndFrame() {
  CHECK(in_frame_);

  Frame& f = frames_[head_];
  f.end = SetMarker();

  f.cpu_duration_ms = MillisecondsFromDuration(f.end.cpu - f.begin.cpu);

  const Frame& prev = frames_[(head_ + kMaxFrames - 1) % kMaxFrames];
  if (prev.begin.cpu != Clock::time_point()) {
    f.cpu_interval_ms = MillisecondsFromDuration(f.begin.cpu - prev.begin.cpu);

    if (f.cpu_interval_ms >
        kNominalFrameIntervalMS + kDroppedFrameAllowanceMS) {
      const int num_frames_dropped =
          static_cast<int>((f.cpu_interval_ms - kDroppedFrameAllowanceMS) /
                           kNominalFrameIntervalMS);
      num_dropped_frames_ += num_frames_dropped;
    }
  }

  f.last_shader.reset();

  gpu_.EndFrame();

  head_ = (head_ + 1) % kMaxFrames;
  in_frame_ = false;
}

void Profiler::RecordDraw(ShaderPtr shader, int num_verts, int num_tris) {
  if (!in_frame_) {
    return;
  }

  Frame& f = frames_[head_];

  if (f.last_shader != shader) {
    ++f.num_shader_swaps;
    f.last_shader = shader;
  }

  ++f.num_draws;
  f.num_verts += num_verts;
  f.num_tris += num_tris;
}

bool Profiler::IsFrameProfiled(const Frame& f) const {
  return (f.cpu_interval_ms != 0.0f &&
          (!GpuProfiler::IsSupported() || f.gpu_interval_ms != 0.0f));
}

float Profiler::GetFrameFps(const Frame& f) const {
  // TODO(b/28473647) use max of cpu / gpu when we get accurate gpu timings.
  // return (1000.0f / std::max(f.cpu_interval_ms, f.gpu_interval_ms));
  return (1.0f / SecondsFromMilliseconds(f.cpu_interval_ms));
}

const Profiler::Frame* Profiler::GetMostRecentProfiledFrame() const {
  for (int i = kMaxFrames - 1; i >= 0; --i) {
    const int index = (head_ + i) % kMaxFrames;
    const Frame& f = frames_[index];
    if (IsFrameProfiled(f)) {
      return &f;
    }
  }
  return nullptr;
}

}  // namespace detail
}  // namespace lull
