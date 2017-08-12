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

#ifndef LULLABY_SYSTEMS_RENDER_RENDER_STATS_H_
#define LULLABY_SYSTEMS_RENDER_RENDER_STATS_H_

#include <memory>
#include <unordered_set>

#include "mathfu/glsl_mappings.h"
#include "lullaby/util/registry.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/render/shader.h"

namespace lull {

class SimpleFont;

class RenderStats {
 public:
  enum class Layer {
    kFpsCounter,   // Onscreen FPS counter.
    kRenderStats,  // Onscreen render stats.
    kTextureSize,  // Checks for potentially erroneous texture sizes.
  };

  struct EnumClassHash {
    template <typename T>
    std::size_t operator()(T t) const {
      return static_cast<std::size_t>(t);
    }
  };

  // Do not create RenderStats directly.  Instead, create via registry, eg:
  // registry.Create<RenderStats>(&registry);
  explicit RenderStats(Registry* registry);

  ~RenderStats();

  // Returns the debug font or nullptr.
  const SimpleFont* GetFont() const { return font_.get(); }

  SimpleFont* GetFont() { return font_.get(); }

  // Returns true if |layer| is enabled.
  bool IsLayerEnabled(Layer layer) const;

  // Sets the status of |layer| to |enabled|.
  void SetLayerEnabled(Layer layer, bool enabled);

  // Starts logging (via INFO) performance stats every |interval| frames.
  // These logs can then be parsed into a CSV by piping through
  // lullaby/scripts/perf_log_to_csv.sh.
  void EnablePerformanceLogging(int interval);

  // Called automatically by RenderSystem.
  void BeginFrame();

  // Called automatically by RenderSystem.
  void EndFrame();

  Registry* registry_;
  std::unordered_set<Layer, EnumClassHash> layers_;
  std::unique_ptr<SimpleFont> font_;
  ShaderPtr font_shader_;
  TexturePtr font_texture_;
  int perf_log_interval_ = 0;
  int perf_log_counter_ = 0;
  int frame_counter_ = 0;
  bool have_logged_headers_ = false;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::RenderStats);

#endif  // LULLABY_SYSTEMS_RENDER_RENDER_STATS_H_
