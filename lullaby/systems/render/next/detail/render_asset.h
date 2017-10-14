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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_DETAIL_RENDER_ASSET_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_DETAIL_RENDER_ASSET_H_

#include <functional>
#include <memory>
#include <string>
#include "lullaby/modules/file/asset.h"

namespace lull {
namespace detail {

/// An Asset that uses finalizes its data to a callback.
class RenderAsset : public Asset,
                    public std::enable_shared_from_this<RenderAsset> {
 public:
  using Finalizer = std::function<void(std::shared_ptr<RenderAsset>)>;

  explicit RenderAsset(Finalizer finalizer) : finalizer(std::move(finalizer)) {}

  void OnFinalize(const std::string& filename, std::string* data) override {
    this->data = std::move(*data);
    finalizer(this->shared_from_this());
  }

  std::string data;
  Finalizer finalizer;
};

}  // namespace detail
}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_DETAIL_RENDER_ASSET_H_
