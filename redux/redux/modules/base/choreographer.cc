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

#include "redux/modules/base/choreographer.h"

#include <cstddef>

namespace redux {

Choreographer::Choreographer(Registry* registry) : registry_(registry) {
  constexpr std::size_t num_stages =
      static_cast<std::size_t>(Stage::kNumStages);

  static int dummy[num_stages * 2] = {0};
  for (std::size_t i = 0; i < (num_stages * 2) - 1; ++i) {
    const Tag t1 = reinterpret_cast<Tag>(&dummy[i]);
    const Tag t2 = reinterpret_cast<Tag>(&dummy[i + 1]);
    graph_.AddDependency(t2, t1);
    if (i % 2 == 0) {
      stage_tags_.emplace_back(t1, t2);
    }
  }
}

void Choreographer::AddDependency(Tag node, Tag dependency) {
  graph_.AddDependency(node, dependency);
}

void Choreographer::Step(absl::Duration delta_time) {
  graph_.Traverse([=](Tag tag) {
    auto iter = handlers_.find(tag);
    if (iter != handlers_.end()) {
      iter->second->Step(registry_, delta_time);
    }
  });
}

void Choreographer::AddToStage(Tag tag, Stage stage) {
  const std::size_t index = static_cast<std::size_t>(stage);
  const std::pair<Tag, Tag> bookends = stage_tags_[index];
  graph_.AddDependency(tag, bookends.first);
  graph_.AddDependency(bookends.second, tag);
}

void Choreographer::Traverse(const std::function<void(std::string_view)>& fn) {
  graph_.Traverse([=](Tag tag) {
    auto iter = handlers_.find(tag);
    if (iter != handlers_.end()) {
      fn(iter->second->GetName());
    }
  });
}

std::string_view Choreographer::HandlerBase::PrettyName(std::string_view name) {
  const auto start = name.find("T = &") + 5;
  const auto end = name.find_last_of("]");
  return name.substr(start, end - start);
}

Choreographer::DependencyBuilder::DependencyBuilder(Choreographer* advancer,
                                                    Tag tag)
    : advancer_(advancer), tag_(tag) {}

}  // namespace redux
