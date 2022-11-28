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

#include "redux/engines/animation/motivator/motivator.h"

#include "redux/engines/animation/common.h"
#include "redux/engines/animation/processor/anim_processor.h"

namespace redux {

Motivator::Motivator(Motivator&& rhs) noexcept {
  if (rhs.Valid()) {
    rhs.processor_->TransferMotivator(rhs.index_, this);
  }
}

Motivator::~Motivator() { Invalidate(); }

Motivator& Motivator::operator=(Motivator&& rhs) noexcept {
  Invalidate();
  if (rhs.Valid()) {
    rhs.processor_->TransferMotivator(rhs.index_, this);
  }
  return *this;
}

void Motivator::Init(AnimProcessor* processor, Index index) {
  // Do not call Invalidate() here.
  processor_ = processor;
  index_ = index;
}

void Motivator::Reset() {
  processor_ = nullptr;
  index_ = kInvalidIndex;
}

void Motivator::Invalidate() {
  if (Valid()) {
    processor_->RemoveMotivator(index_);
  }
}

bool Motivator::Valid() const {
  return processor_ != nullptr && index_ != kInvalidIndex;
}

bool Motivator::Sane() const {
  return (processor_ == nullptr && index_ == kInvalidIndex) ||
         (processor_ != nullptr && processor_->ValidMotivator(index_, this));
}

int Motivator::Dimensions() const { return processor_->Dimensions(index_); }

void Motivator::CloneFrom(const Motivator* other) {
  Invalidate();
  if (other && other->Valid()) {
    other->processor_->CloneMotivator(this, other->index_);
  }
}

}  // namespace redux
