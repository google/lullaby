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

#include "lullaby/systems/render/render_stats.h"

#include "lullaby/systems/render/detail/profiler.h"
#include "lullaby/systems/render/simple_font.h"
#include "lullaby/util/math.h"

namespace lull {
namespace {
bool DoesLayerNeedProfiler(RenderStats::Layer layer) {
  return (layer == RenderStats::Layer::kFpsCounter ||
          layer == RenderStats::Layer::kRenderStats);
}
}  // namespace

constexpr const char* kFontShader = "shaders/texture.fplshader";
constexpr const char* kFontTexture = "textures/debug_font.webp";

RenderStats::RenderStats(Registry* registry) : registry_(registry) {
  auto* render_system = registry_->Get<RenderSystem>();
  font_shader_ = render_system->LoadShader(kFontShader);
  font_texture_ = render_system->LoadTexture(kFontTexture);
  font_.reset(new SimpleFont(font_shader_, font_texture_));
}

RenderStats::~RenderStats() {}

bool RenderStats::IsLayerEnabled(Layer layer) const {
  return (layers_.find(layer) != layers_.end());
}

void RenderStats::SetLayerEnabled(Layer layer, bool enabled) {
  if (enabled) {
    detail::Profiler* profiler = registry_->Get<detail::Profiler>();
    if (!profiler && DoesLayerNeedProfiler(layer)) {
      registry_->Create<detail::Profiler>();
    }

    layers_.emplace(layer);
  } else {
    layers_.erase(layer);
  }
}

void RenderStats::EnablePerformanceLogging(int interval) {
  DCHECK_GT(interval, 0) << "Invalid interval " << interval;
  perf_log_interval_ = interval;
  perf_log_counter_ = interval;

  if (!registry_->Get<detail::Profiler>()) {
    registry_->Create<detail::Profiler>();
  }
}

void RenderStats::BeginFrame() {
  ++frame_counter_;

  detail::Profiler* profiler = registry_->Get<detail::Profiler>();
  if (profiler) {
    profiler->BeginFrame();
  }
}

void RenderStats::EndFrame() {
  detail::Profiler* profiler = registry_->Get<detail::Profiler>();
  if (profiler) {
    profiler->EndFrame();
  }

  if (profiler && perf_log_interval_ > 0 && --perf_log_counter_ == 0) {
    // Print out in CSV-ready format.
    if (!have_logged_headers_) {
      LOG(INFO) << "LullPerf frame #, FPS, CPU, GPU, # draws,"
                   " # shader swaps, # verts, # tris";
      have_logged_headers_ = true;
    }

    LOG(INFO) << "LullPerf " << frame_counter_ << ", "
              << profiler->GetFilteredFps() << ", " << profiler->GetCpuFrameMs()
              << ", " << profiler->GetGpuFrameMs() << ", "
              << profiler->GetNumDraws() << ", "
              << profiler->GetNumShaderSwaps() << ", "
              << profiler->GetNumVerts() << ", " << profiler->GetNumTris();
    perf_log_counter_ = perf_log_interval_;
  }
}

}  // namespace lull
