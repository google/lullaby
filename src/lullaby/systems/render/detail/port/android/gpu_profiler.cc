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

#include <string.h>

#include <EGL/egl.h>

#include "lullaby/util/logging.h"

// The FPL and Ion toolchains use different GL header include paths, so we need
// to include them explicitly for consistency.
#include "GLES2/gl2.h"

// The Android emulator can't resolve the disjoint timer extension functions
// when they're declared as prototypes, so we need to retrieve them ourselves
// from EGL.
#ifdef GL_GLEXT_PROTOTYPES
#undef GL_GLEXT_PROTOTYPES
#endif
#include "GLES2/gl2ext.h"

namespace lull {
namespace detail {

PFNGLGENQUERIESEXTPROC glGenQueriesEXT = NULL;
PFNGLDELETEQUERIESEXTPROC glDeleteQueriesEXT = NULL;
PFNGLISQUERYEXTPROC glIsQueryEXT = NULL;
PFNGLBEGINQUERYEXTPROC glBeginQueryEXT = NULL;
PFNGLENDQUERYEXTPROC glEndQueryEXT = NULL;
PFNGLQUERYCOUNTEREXTPROC glQueryCounterEXT = NULL;
PFNGLGETQUERYOBJECTIVEXTPROC glGetQueryObjectivEXT = NULL;
PFNGLGETQUERYOBJECTUI64VEXTPROC glGetQueryObjectui64vEXT = NULL;

GpuProfiler::GpuProfiler() {
  if (IsSupported()) {
    static const int kNumInitialQueries = 4;
    Query pool[kNumInitialQueries];
    glGenQueriesEXT(kNumInitialQueries, pool);
    for (int i = 0; i < kNumInitialQueries; ++i) {
      available_.push(pool[i]);
    }

    // Clear disjoint flag.
    GLint disjoint = 0;
    glGetIntegerv(GL_GPU_DISJOINT_EXT, &disjoint);
  }
}

GpuProfiler::~GpuProfiler() {
  while (!pending_.empty()) {
    Query query = pending_.front();
    glDeleteQueriesEXT(1, &query);
    pending_.pop_front();
  }

  while (!available_.empty()) {
    Query query = available_.front();
    glDeleteQueriesEXT(1, &query);
    available_.pop();
  }

  for (Query query : abandoned_) {
    glDeleteQueriesEXT(1, &query);
  }

  for (auto it : ready_) {
    glDeleteQueriesEXT(1, &it.first);
  }

  for (Query query : active_timers_) {
    glDeleteQueriesEXT(1, &query);
  }
}

bool GpuProfiler::IsActiveTimer(Query query) const {
  bool found = false;
  for (size_t i = 0; !found && i < active_timers_.size(); ++i) {
    found = (active_timers_[i] == query);
  }
  return found;
}

bool GpuProfiler::GetTime(Query query, uint64_t *out_nanoseconds) {
  auto it = ready_.find(query);
  if (it != ready_.end()) {
    *out_nanoseconds = it->second;
    ready_.erase(it);
    available_.push(query);
    return true;
  }
  return false;
}

void GpuProfiler::Abandon(Query query) {
  if (query == kInvalidQuery) {
    return;
  }

  DCHECK(!IsActiveTimer(query)) << "Can't abandon an active timer.";
  DCHECK(glIsQueryEXT(query));

  // Mark the query as abandoned only if it's still pending.
  if (std::find(pending_.begin(), pending_.end(), query) != pending_.end()) {
    abandoned_.emplace(query);

#ifdef LULLABY_GPU_PROFILER_LOG_USAGE
    LOG(INFO) << "GpuProfiler abandoned pending query " << query;
#endif
  } else if (ready_.erase(query) > 0) {
    available_.push(query);

#ifdef LULLABY_GPU_PROFILER_LOG_USAGE
    LOG(INFO) << "GpuProfiler abandoned ready query " << query;
#endif
  } else {
#ifdef LULLABY_GPU_PROFILER_LOG_USAGE
    const char *adjective = "unknown";
    if (abandoned_.find(query) != abandoned_.end()) {
      adjective = "abandoned";
    }

    LOG(WARNING) << "GpuProfiler tried to abandon " << adjective << " query "
                 << query;
#endif
  }
}

GpuProfiler::Query GpuProfiler::GetAvailableQuery() {
  Query query = kInvalidQuery;
  if (!available_.empty()) {
    query = available_.front();
    available_.pop();
  } else {
    glGenQueriesEXT(1, &query);
  }
  return query;
}

GpuProfiler::Query GpuProfiler::SetMarker() {
  if (!IsSupported()) {
    return kInvalidQuery;
  }

  Query query = GetAvailableQuery();
  if (query != kInvalidQuery) {
    glQueryCounterEXT(query, GL_TIMESTAMP_EXT);
    pending_.push_back(query);

#ifdef LULLABY_GPU_PROFILER_LOG_USAGE
    LOG(INFO) << "GpuProfiler set marker " << query;
#endif
  }
  return query;
}

GpuProfiler::Query GpuProfiler::BeginTimer() {
  if (!IsSupported()) {
    return kInvalidQuery;
  }

  Query query = GetAvailableQuery();
  if (query != kInvalidQuery) {
    glBeginQueryEXT(GL_TIME_ELAPSED_EXT, query);
    active_timers_.push_back(query);

#ifdef LULLABY_GPU_PROFILER_LOG_USAGE
    LOG(INFO) << "GpuProfiler begin timer " << query;
#endif
  }
  return query;
}

void GpuProfiler::EndTimer(Query query) {
  if (query != kInvalidQuery) {
    CHECK_EQ(query, active_timers_.back());
    glEndQueryEXT(GL_TIME_ELAPSED_EXT);
    active_timers_.pop_back();
    pending_.push_back(query);

#ifdef LULLABY_GPU_PROFILER_LOG_USAGE
    LOG(INFO) << "GpuProfiler end timer " << query;
#endif
  }
}

void GpuProfiler::PollQueries() {
  // Adapted from ion::gfxprofile::GpuProfiler::PollGlTimerQueries.
  if (!IsSupported()) {
    return;
  }

  bool has_checked_disjoint = false;
  bool was_disjoint = false;
  for (;;) {
    if (pending_.empty()) {
      // No queries pending.
      return;
    }

    Query query = pending_.front();

    GLint available = 0;
    glGetQueryObjectivEXT(query, GL_QUERY_RESULT_AVAILABLE_EXT, &available);
    if (!available) {
      // No queries available.
      return;
    }

    // Found an available query, remove it from pending queue.
    pending_.pop_front();

    if (!has_checked_disjoint) {
      // Check if we need to ignore the result of the timer query because
      // of some kind of disjoint GPU event such as heat throttling.
      // If so, we ignore all events that are available during this loop.
      has_checked_disjoint = true;
      GLint disjoint_occurred = 0;
      glGetIntegerv(GL_GPU_DISJOINT_EXT, &disjoint_occurred);
      was_disjoint = (disjoint_occurred != 0);
      if (was_disjoint) {
        LOG(WARNING) << "Skipping disjoint GPU events";
      }
    }

    auto abandoned_it = abandoned_.find(query);
    if (abandoned_it != abandoned_.end()) {
      available_.push(query);
      abandoned_.erase(abandoned_it);
#ifdef LULLABY_GPU_PROFILER_LOG_USAGE
      LOG(INFO) << "GpuProfiler finished abandoned query " << query;
#endif
      continue;
    }

    uint64_t elapsed = 0;
    if (!was_disjoint) {
      glGetQueryObjectui64vEXT(query, GL_QUERY_RESULT_EXT, &elapsed);
    }
    ready_[query] = elapsed;

#ifdef LULLABY_GPU_PROFILER_LOG_USAGE
    LOG(INFO) << "GpuProfiler resolved query " << query;
#endif
  }
}

void GpuProfiler::BeginFrame() { PollQueries(); }

void GpuProfiler::EndFrame() {}

#define LOOKUP_GL_FUNC(type, name) name = (type)eglGetProcAddress(#name)

bool GpuProfiler::IsSupported() {
  static const bool available = []() {
#if ION_PRODUCTION
    return false;
#endif
    const GLubyte *ext = glGetString(GL_EXTENSIONS);
    if (!ext ||
        strstr(reinterpret_cast<const char *>(ext),
               "GL_EXT_disjoint_timer_query") == NULL) {
      return false;
    }
    LOG(INFO) << "Found disjoint timer extension.";
    LOOKUP_GL_FUNC(PFNGLGENQUERIESEXTPROC, glGenQueriesEXT);
    LOOKUP_GL_FUNC(PFNGLDELETEQUERIESEXTPROC, glDeleteQueriesEXT);
    LOOKUP_GL_FUNC(PFNGLISQUERYEXTPROC, glIsQueryEXT);
    LOOKUP_GL_FUNC(PFNGLBEGINQUERYEXTPROC, glBeginQueryEXT);
    LOOKUP_GL_FUNC(PFNGLENDQUERYEXTPROC, glEndQueryEXT);
    LOOKUP_GL_FUNC(PFNGLQUERYCOUNTEREXTPROC, glQueryCounterEXT);
    LOOKUP_GL_FUNC(PFNGLGETQUERYOBJECTIVEXTPROC, glGetQueryObjectivEXT);
    LOOKUP_GL_FUNC(PFNGLGETQUERYOBJECTUI64VEXTPROC, glGetQueryObjectui64vEXT);
    return true;
  }();
  return available;
}

}  // namespace detail
}  // namespace lull
