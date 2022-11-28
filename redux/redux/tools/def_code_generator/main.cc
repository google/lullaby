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

#include <cctype>
#include <string>

#include "redux/tools/def_code_generator/generate_code.h"
#include "redux/tools/def_code_generator/parse_def_file.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "redux/modules/base/filepath.h"
#include "redux/modules/base/logging.h"
#include "redux/tools/common/file_utils.h"

ABSL_FLAG(std::string, input, "", "Input def file.");
ABSL_FLAG(std::string, output, "", "Output header file.");

namespace redux::tool {

static int RunDefCodeGenerator() {
  const std::string input = absl::GetFlag(FLAGS_input);
  CHECK(!input.empty()) << "Must specify input file.";
  CHECK(GetExtension(input) == ".def") << "Input must be a .def file.";

  const std::string output = absl::GetFlag(FLAGS_output);

  const std::string contents = LoadFileAsString(input.c_str());
  CHECK(!contents.empty()) << "Input file cannot be loaded or is empty!";

  const absl::StatusOr<DefDocument> doc = ParseDefFile(contents);
  CHECK(doc.ok()) << doc.status();

  const std::string code = GenerateCode(doc.value());

  CHECK(SaveFile(code.data(), code.size(), output.c_str(), false))
      << "Failed to save to file: " << output;
  return 0;
}

}  // namespace redux::tool

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  return redux::tool::RunDefCodeGenerator();
}
