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

#ifndef LULLABY_UTIL_PROFILER_H_
#define LULLABY_UTIL_PROFILER_H_

/// @file
/// The profiler header provides functions and macros to profile parts of your
/// code.
///
/// The easiest way to sample a code segment is by using the LULL_PROFILE macro.
/// void my_function() {
///   LULL_PROFILE(my_function);
///   // Function login here.
/// }
///
/// This effectively calls LULL_PROFILE_START(my_function) at the macro call
/// and LULL_PROFILE_END(my_function) at the end of the scope.
///
/// To retrieve the data, use GetProfilerData(). Note that profiling data is
/// thread specific and will not return data from different threads.
///
/// You can print the return value of GetProfilerData() to a stream object, this
/// works nicely with logging. Dump this to log like so:
/// LOG(INFO) << *GetProfilerData();
///
/// Note that samples of recursion functions will include the time for the
/// entire duration of the first call to the function including all its
/// recursive calls.

#include <chrono>
#include <iostream>
#include <stack>
#include <vector>

#include "lullaby/util/string_view.h"


namespace lull {
namespace detail {

/// Marks the start point of a sample for profiling. This should not be used
/// directly and is only public so the macros can access it.
void ProfileSampleStart(const char* sample_name, size_t* index);

/// Marks the end point of a sample for profiling. This should not be used
/// directly and is only public so the macros can access it.
void ProfileSampleEnd(size_t* index);

}  // namespace detail

/// Constant value defining the ID of uninitialized profile samples.
static const size_t kUninitializedProfileSampleIndex = size_t(-1);
/// Constant value defining the maximum number of samples in each profiler.
static const size_t kProfilerMaxSamples = 4000;

/// Cleans the profiler data for the thread it was called.
void CleanupProfiler();

/// ProfilerSampleData holds the data of a sample being profiled. A sample is
/// defined as a segment of code between calls to |LULL_PROFILE_START| and
/// |LULL_PROFILE_END| where the |sample_name| value is identical in both calls.
/// Samples can also be defined by used the |SampleProfiler| class and
/// |LULL_PROFILE| macro. Note that |LULL_PROFILE_END| must be called within the
/// same scope as |LULL_PROFILE_START|.
struct ProfilerSampleData {
  /// The name of this sample.
  string_view name;
  /// The time point at which the sample started.
  std::chrono::time_point<std::chrono::high_resolution_clock> start_time_point;
  /// The time point at which the sample ended.
  std::chrono::time_point<std::chrono::high_resolution_clock> end_time_point;
  /// The index of the profile sample called before this one.
  size_t parent_index = 0;
  /// Number of times this sample was called.
  size_t times_called = 0;
  /// How many times has this sample started without finishing?
  size_t current_unfinished_call = 0;
};

/// Profiler data holds all the data collected by the profiler on a given
/// thread.
struct ProfilerData {
  /// Array of the samples being profiled.
  std::vector<ProfilerSampleData> samples;
  /// Index of the current sample being processed.
  size_t current_sample_index = 0;
  /// The next allocated index is used to give an index for new samples.
  size_t next_allocated_index = 0;

  ProfilerData() : samples(kProfilerMaxSamples) {}
};

/// Retrieves the profiler data for the current thread.
const ProfilerData* GetProfilerData();

/// The ScopedSampleProfiler is a helper class for profiling samples of code. It
/// calls |ProfileSampleStart| at its construction and |ProfileSampleEnd| at its
/// destruction.
class ScopedSampleProfiler {
 public:
  /// The constructor initializes the profiling of a sample by calling
  /// |ProfileSampleStart| with the |sample_name|.
  explicit ScopedSampleProfiler(const char* sample_name, size_t* index)
      : index_(index) {
    detail::ProfileSampleStart(sample_name, index_);
  }

  /// The destructor marks the end for profiling a sample by calling
  /// |ProfileSampleEnd|.
  ~ScopedSampleProfiler() { detail::ProfileSampleEnd(index_); }

 private:
  // Pointer to the index value for the sampler.
  size_t* index_;

  ScopedSampleProfiler(const ScopedSampleProfiler& other) = delete;
};

/// Operator to stream the profiler, used to print out to log / console.
std::ostream& operator<<(std::ostream& os, const ProfilerData& profiler);

}  // namespace lull


#endif  // LULLABY_UTIL_PROFILER_H_
