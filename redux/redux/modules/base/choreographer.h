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

#ifndef REDUX_MODULES_BASE_CHOREOGRAPHER_H_
#define REDUX_MODULES_BASE_CHOREOGRAPHER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/time/time.h"
#include "redux/modules/base/dependency_graph.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/base/typeid.h"

#if !defined(__PRETTY_FUNCTION__) && defined(__FUNCSIG__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

namespace redux {

// Responsible for "stepping" objects in the registry in the correct order.
//
// Objects register a step compile-time known function with the choreographer
// along with information about when that function should be called. Then all
// objects can be stepped in the correct order by calling Choreographer::Step.
//
// The choreographer is broken down into multiple stages which are stepped in
// order. This provides a high-level ordering against which functions can be
// registered.
//
// More fine-grained ordering can be specified by explicitly registering a
// function to be called either before or after another function.
class Choreographer {
 public:
  explicit Choreographer(Registry* registry);

  Choreographer(const Choreographer&) = delete;
  Choreographer& operator=(const Choreographer&) = delete;

  // A single step is composed of the following stages. Every function that
  // is registered to update in a step must occur within one of these stages.
  // These stages are stepped in this order.
  enum class Stage {
    kPrologue,
    kInput,
    kEvents,
    kLogic,
    kAnimation,
    kPhysics,
    kPostPhysics,
    kRender,
    kEpilogue,
    kNumStages,
  };

  // Each function is automatically assigned a unique Tag. For the most part,
  // users of this class should not concern themselves with this Tag.
  using Tag = uintptr_t;

  // Calls all the registered functions in order, passing them the provided
  // delta_time if applicable.
  void Step(absl::Duration delta_time);

  // A proxy class that can be used to provide more fine-grained control over
  // update ordering. An instance of this class is returned by
  // Choreographer::Add after which the Before/After functions can be used
  // to create dependencies with other functions.
  class DependencyBuilder {
   public:
    DependencyBuilder() = default;
    DependencyBuilder(Choreographer* advancer, Tag tag);

    // Requires that the function registered with Add() is stepped before the
    // function provided here.
    template <auto Fn>
    DependencyBuilder& Before();

    // Requires that the function registered with Add() is stepped after the
    // function provided here.
    template <auto Fn>
    DependencyBuilder& After();

   private:
    Choreographer* advancer_ = nullptr;
    Tag tag_ = 0;
  };

  // Adds the compile-time known function such that it will be stepped during
  // the given stage. A function should only be registered once.
  template <auto Fn>
  DependencyBuilder Add(Stage stage) {
    return DependencyBuilder(this, Register<Fn>(stage));
  }

  // Traverses the registered functions in order, returning their names. This
  // is useful for debugging purposes.
  void Traverse(const std::function<void(std::string_view)>& fn);

 private:
  using StepFn = std::function<void(absl::Duration)>;

  struct HandlerBase {
    virtual ~HandlerBase() = default;
    virtual std::string_view GetName() const = 0;
    virtual void Step(Registry* registry, absl::Duration) = 0;
    static std::string_view PrettyName(std::string_view name);
  };

  template <auto T>
  struct Handler;

  template <typename R, typename T, R (T::*Fn)()>
  struct Handler<Fn> : HandlerBase {
    static Tag GetTag() {
      static int dummy = 0;
      return reinterpret_cast<Tag>(&dummy);
    }
    std::string_view GetName() const override {
      return PrettyName(__PRETTY_FUNCTION__);
    }
    void Step(Registry* registry, absl::Duration dt) override {
      T* obj = registry->Get<T>();
      if (obj) {
        (obj->*Fn)();
      }
    }
  };

  template <typename R, typename T, R (T::*Fn)(absl::Duration)>
  struct Handler<Fn> : HandlerBase {
    static Tag GetTag() {
      static int dummy = 0;
      return reinterpret_cast<Tag>(&dummy);
    }
    std::string_view GetName() const override {
      return PrettyName(__PRETTY_FUNCTION__);
    }
    void Step(Registry* registry, absl::Duration dt) override {
      T* obj = registry->Get<T>();
      if (obj) {
        (obj->*Fn)(dt);
      }
    }
  };

  template <auto Fn>
  static Tag GetTag() {
    return Handler<Fn>::GetTag();
  }

  template <auto Fn>
  Tag Register(Stage stage) {
    const Tag tag = GetTag<Fn>();
    if (auto& h = handlers_[tag]; h == nullptr) {
      graph_.AddNode(tag);
      h = std::make_unique<Handler<Fn>>();
      AddToStage(tag, stage);
    }
    return tag;
  }

  void AddDependency(Tag node, Tag dependency);

  void AddToStage(Tag tag, Stage stage);

  Registry* registry_;
  DependencyGraph<Tag> graph_;
  absl::flat_hash_map<Tag, std::unique_ptr<HandlerBase>> handlers_;
  std::vector<std::pair<Tag, Tag>> stage_tags_;
};

template <auto Fn>
Choreographer::DependencyBuilder& Choreographer::DependencyBuilder::Before() {
  if (advancer_) {
    advancer_->AddDependency(Choreographer::GetTag<Fn>(), tag_);
  }
  return *this;
}

template <auto Fn>
Choreographer::DependencyBuilder& Choreographer::DependencyBuilder::After() {
  if (advancer_) {
    advancer_->AddDependency(tag_, Choreographer::GetTag<Fn>());
  }
  return *this;
}

}  // namespace redux

REDUX_SETUP_TYPEID(redux::Choreographer);

#endif  // REDUX_MODULES_BASE_CHOREOGRAPHER_H_
