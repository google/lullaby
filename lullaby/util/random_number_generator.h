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

#ifndef LULLABY_UTIL_RANDOM_NUMBER_GENERATOR_H_
#define LULLABY_UTIL_RANDOM_NUMBER_GENERATOR_H_

#include <random>

#include "lullaby/util/typeid.h"

namespace lull {

// A class for generating random numbers at various distributions using the
// std::mt19937 random number generation engine.
class RandomNumberGenerator {
 public:
  RandomNumberGenerator() : rng_engine_(std::random_device()()) {}

  // Generate a uniformly random int between |min| and |max|, inclusive.
  int GenerateUniform(int min, int max) {
    return std::uniform_int_distribution<int>(min, max)(rng_engine_);
  }

  // Generate a uniformly random float between |min| and |max|, inclusive.
  float GenerateUniform(float min, float max) {
    return std::uniform_real_distribution<float>(min, max)(rng_engine_);
  }

 private:
  std::mt19937 rng_engine_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::RandomNumberGenerator);

#endif  // LULLABY_UTIL_RANDOM_NUMBER_GENERATOR_H_
