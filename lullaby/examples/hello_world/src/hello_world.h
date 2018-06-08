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

#ifndef LULLABY_EXAMPLES_HELLO_WORLD_SRC_HELLO_WORLD_H_
#define LULLABY_EXAMPLES_HELLO_WORLD_SRC_HELLO_WORLD_H_

#include "lullaby/examples/example_app/example_app.h"

// A simple example of Lullaby VR app.
class HelloWorld : public lull::ExampleApp {
 public:
  HelloWorld() {}

 protected:
  void OnInitialize() override;
  void OnAdvanceFrame(lull::Clock::duration delta_time) override;
  void OnRender(lull::Span<lull::RenderView> views) override;
};

#endif  // LULLABY_EXAMPLES_HELLO_WORLD_SRC_HELLO_WORLD_H_
