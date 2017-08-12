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

#include "lullaby/util/profiler.h"

#include <iomanip>

#include "lullaby/util/logging.h"

#ifdef __APPLE__
#include "TargetConditionals.h"  // This will define TARGET_OS_IPHONE on iOS.
#endif

namespace lull {
namespace {

// The iOS compiler we have with Blaze does not support thread local storage
// so we disable the functionality on iOS.
#ifndef TARGET_OS_IPHONE
thread_local ProfilerData* g_profiler_data = nullptr;
#endif

ProfilerData* GetMutableProfilerData() {
#ifdef TARGET_OS_IPHONE
  return nullptr;
#else
  if (!g_profiler_data) {
    g_profiler_data = new ProfilerData();
  }

  return g_profiler_data;
#endif
}

ProfilerSampleData* GetSampleData(size_t* index,
                                  const char* sample_name = nullptr) {
  ProfilerData* profile = GetMutableProfilerData();

  if (*index == kUninitializedProfileSampleIndex) {
    *index = profile->next_allocated_index++;
    profile->samples[*index].name = sample_name;
  }

  return &profile->samples[*index];
}

}  // namespace

namespace detail {

void ProfileSampleStart(const char* sample_name, size_t* index) {
#ifdef TARGET_OS_IPHONE
  return;
#endif
  ProfilerData* profiler = GetMutableProfilerData();
  ProfilerSampleData* sample = GetSampleData(index, sample_name);

  ++sample->times_called;
  sample->parent_index = profiler->current_sample_index;
  profiler->current_sample_index = *index;

  if (sample->current_unfinished_call == 0) {
    sample->start_time_point = std::chrono::high_resolution_clock::now();
  }
  ++sample->current_unfinished_call;
}

void ProfileSampleEnd(size_t* index) {
#ifdef TARGET_OS_IPHONE
  return;
#endif
  ProfilerData* profiler = GetMutableProfilerData();
  ProfilerSampleData* sample = GetSampleData(index);

  CHECK_GT(sample->current_unfinished_call, 0);
  --sample->current_unfinished_call;
  if (sample->current_unfinished_call == 0) {
    sample->end_time_point = std::chrono::high_resolution_clock::now();
    profiler->current_sample_index = sample->parent_index;
  }
}

}  // namespace detail

const ProfilerData* GetProfilerData() { return GetMutableProfilerData(); }

void CleanupProfiler() {
#ifndef TARGET_OS_IPHONE
  delete g_profiler_data;
  g_profiler_data = nullptr;
#endif
}

std::ostream& operator<<(std::ostream& os, const ProfilerData& profiler) {
  os << std::endl;
  os << std::setw(40) << "Sample Name "
     << " | ";
  os << std::setw(15) << "Runtime (ms) "
     << " | ";
  os << std::setw(15) << "Call Count "
     << " | ";
  os << std::endl;

  size_t count = 0;
  for (auto sample : profiler.samples) {
    if (count++ == profiler.next_allocated_index) break;
    if (sample.current_unfinished_call > 0) {
      continue;
    }

    os << std::setw(40) << sample.name.data() << " | ";
    os << std::setw(15) << std::fixed
       << (std::chrono::duration<double, std::milli>(sample.end_time_point -
                                                     sample.start_time_point))
              .count()
       << " | ";
    os << std::setw(15) << sample.times_called << " | ";
    os << std::endl;
  }
  os << "----------------------------------------------------------------------"
        "--------";
  os << std::endl;

  return os;
}

}  // namespace lull
