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

#ifndef LULLABY_SYSTEMS_RENDER_DETAIL_GPU_PROFILER_H_
#define LULLABY_SYSTEMS_RENDER_DETAIL_GPU_PROFILER_H_

#include <stdint.h>

#include <deque>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace lull {
namespace detail {

// GpuProfiler provides a way to place markers in the command stream that record
// the GPU timestamps at specific events.  It also provides a way to place
// begin/end markers to record the elapsed GPU time between two events.
class GpuProfiler {
 public:
  using Query = uint32_t;
  enum { kInvalidQuery = 0 };

  GpuProfiler();
  ~GpuProfiler();

  // Returns true if |query| is finished.  If |query| was successful,
  // |out_nanoseconds| holds either the time (for markers), or elapsed time (for
  // timers).  If |query| was unsuccessful, |out_nanoseconds| is set to 0.
  bool GetTime(Query query, uint64_t *out_nanoseconds);

  // Releases |query| without regard for the result.  Queries are only released
  // by a successful call to GetTime() or Abandon().
  void Abandon(Query query);

  // Returns a marker or kInvalidQuery.  If valid, the user owns this query
  // until a successful call to GetTime() or Abandon().
  Query SetMarker();

  // Starts and returns a timer query, or kInvalidQuery.  If valid, the user
  // owns this query until a successful call to GetTime() or Abandon().
  Query BeginTimer();

  // Ends the timer query.
  void EndTimer(Query query);

  // Performs beginning-of-frame operations.
  void BeginFrame();

  // Performs end-of-frame operations.
  void EndFrame();

  // Returns true if the GPU profiler is supported on the current device.
  static bool IsSupported();

 private:
  Query GetAvailableQuery();
  void PollQueries();
  bool IsActiveTimer(Query query) const;

  // A pool of pending queries that we're waiting to receive times for.
  std::deque<Query> pending_;

  // A pool of unused queries.
  std::queue<Query> available_;

  // A pool of abandoned queries that are still pending.  Once an abandoned
  // query is completed, it is immediately made available.
  std::unordered_set<Query> abandoned_;

  // A map of queries and their times reported by the GPU.  Queries from
  // pending_ are potentially promoted to ready_ in BeginFrame.  Once
  // someone retrieves a query's time in GetTime, it is moved from ready_ to
  // available_.
  std::unordered_map<Query, uint64_t> ready_;

  // Stack of active timers (BeginTimer / EndTimer pairs).  Stored as a vector
  // since we need to search through it.
  std::vector<Query> active_timers_;

  GpuProfiler(const GpuProfiler&) = delete;
  GpuProfiler& operator=(const GpuProfiler&) = delete;
};

}  // namespace detail
}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_DETAIL_GPU_PROFILER_H_
