/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#include <string>

#include "gtest/gtest.h"
#include "lullaby/tools/shader_pipeline/process_shader_source.h"

namespace lull {
namespace {

TEST(ShaderProcessorTest, NoOp) {
  const std::string shader_string =
      "void main() {\n"
      "  return vec4(1.0, 1.0, 1.0, 1.0);\n"
      "}\n";

  std::string processed_shader_string = shader_string;
  EXPECT_TRUE(tool::ProcessShaderSource(&processed_shader_string));

  EXPECT_EQ(shader_string, processed_shader_string);
}

TEST(ShaderProcessorTest, PlainInclude) {
  std::string shader_string =
      "#include "
      "\"lullaby/tests/data/"
      "shader_processor_test_include_0.glslh\"";
  EXPECT_TRUE(tool::ProcessShaderSource(&shader_string));

  const std::string expected_result =
      "int IncludedFoo_0(int x, int y) {\n"
      "  return x + y;\n"
      "}\n";

  EXPECT_EQ(expected_result, shader_string);
}

TEST(ShaderProcessorTest, Include) {
  std::string shader_string =
      "#include "
      "\"lullaby/tests/data/"
      "shader_processor_test_include_0.glslh\"\n"
      "void main() {\n"
      "  return vec4(1.0, 1.0, 1.0, 1.0);\n"
      "}\n";
  EXPECT_TRUE(tool::ProcessShaderSource(&shader_string));

  const std::string expected_result =
      "int IncludedFoo_0(int x, int y) {\n"
      "  return x + y;\n"
      "}\n"
      "\n"
      "void main() {\n"
      "  return vec4(1.0, 1.0, 1.0, 1.0);\n"
      "}\n";

  EXPECT_EQ(expected_result, shader_string);
}

TEST(ShaderProcessorTest, NestedInclude) {
  std::string shader_string =
      "#include "
      "\"lullaby/tests/data/"
      "shader_processor_test_include_1.glslh\"";
  EXPECT_TRUE(tool::ProcessShaderSource(&shader_string));

  const std::string expected_result =
      "int IncludedFoo_0(int x, int y) {\n"
      "  return x + y;\n"
      "}\n"
      "\n"
      "\n"
      "int IncludedFoo_1(int x, int y) {\n"
      "  return x * y;\n"
      "}\n";

  EXPECT_EQ(expected_result, shader_string);
}

TEST(ShaderProcessorTest, BadInclude) {
  const std::string expected_result =
      "oops#include "
      "\"lullaby/tests/data/"
      "shader_processor_test_include_0.glslh\"";
  std::string shader_string = expected_result;

  EXPECT_TRUE(tool::ProcessShaderSource(&shader_string));
  EXPECT_EQ(expected_result, shader_string);
}

}  // namespace
}  // namespace lull
