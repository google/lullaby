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

#include "lullaby/systems/render/detail/gpu_profiler.h"

namespace lull {
namespace detail {

GpuProfiler::GpuProfiler() {}

GpuProfiler::~GpuProfiler() {}

bool GpuProfiler::GetTime(Query q, uint64_t *out_nanoseconds) { return false; }

void GpuProfiler::Abandon(Query query) {}

GpuProfiler::Query GpuProfiler::SetMarker() { return kInvalidQuery; }

GpuProfiler::Query GpuProfiler::BeginTimer() { return kInvalidQuery; }

void GpuProfiler::EndTimer(Query t) {}

void GpuProfiler::BeginFrame() {}

void GpuProfiler::EndFrame() {}

GpuProfiler::Query GpuProfiler::GetAvailableQuery() { return kInvalidQuery; }

void GpuProfiler::PollQueries() {}

bool GpuProfiler::IsActiveTimer(Query query) const { return false; }

bool GpuProfiler::IsSupported() { return false; }

}  // namespace detail
}  // namespace lull
