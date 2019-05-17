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

#include "lullaby/util/arg_parser.h"
#include "lullaby/util/filename.h"
#include "lullaby/util/logging.h"
#include "lullaby/tools/common/file_utils.h"
#include "lullaby/tools/compile_blueprint_from_json/blueprint_from_json_compiler.h"

// This tool reads fbs files defining all known ComponentDefs, compiles JSON
// files that use those ComponentDefs into BlueprintDefs, and saves them.

namespace lull {
namespace tool {
namespace {

std::string GetOutputName(string_view output_path, string_view name) {
  std::string output_file =
      RemoveDirectoryAndExtensionFromFilename(std::string(name)) + ".bin";
  if (output_path.empty()) {
    return output_file;
  } else {
    return JoinPath(output_path, output_file);
  }
}

// Loads the files from |fbs_srcs| to define the available schema.
// |include_paths| is used to resolve any include statements.
bool ParseFbses(BlueprintFromJsonCompiler* compiler, Span<string_view> fbs_srcs,
                Span<string_view> include_paths) {
  std::vector<std::string> include_strings;
  std::vector<const char*> include_ptrs;
  include_strings.reserve(include_paths.size());
  include_ptrs.reserve(include_paths.size() + 1);
  for (auto include_path : include_paths) {
    include_strings.emplace_back(include_path);
    include_ptrs.emplace_back(include_strings.back().c_str());
  }
  // Null-terminate the list of cstrings itself.
  include_ptrs.emplace_back(nullptr);

  for (auto fbs_src : fbs_srcs) {
    const std::string fbs_src_string(fbs_src);
    std::string contents;
    if (!LoadFile(fbs_src_string.c_str(), false, &contents)) {
      LOG(ERROR) << "Unable to read file: " << fbs_src;
      return false;
    }
    if (!compiler->ParseFbs(contents, &include_ptrs[0], fbs_src_string)) {
      return false;
    }
  }
  return true;
}

// For each file in |json_srcs|, compiles the flatbuffer binary and saves it
// using the same filename but replacing the extension with ".bin". If
// |output_path| is empty it uses the current directory.
bool ParseJsons(BlueprintFromJsonCompiler* compiler,
                Span<string_view> json_srcs, string_view output_path) {
  for (auto json_src : json_srcs) {
    const std::string json_src_string(json_src);
    std::string contents;
    if (!LoadFile(json_src_string.c_str(), false, &contents)) {
      LOG(ERROR) << "Unable to read file: " << json_src;
      return false;
    }

    flatbuffers::DetachedBuffer buffer = compiler->ParseJson(contents);
    if (buffer.size() == 0) {
      LOG(ERROR) << "Error while parsing json: " << json_src;
      return false;
    }

    std::string output_name = GetOutputName(output_path, json_src);
    if (!SaveFile(reinterpret_cast<char*>(buffer.data()), buffer.size(),
                  output_name.c_str(), true)) {
      LOG(ERROR) << "Error saving file: " << output_name;
      return false;
    }
  }
  return true;
}

}  // namespace

int Run(int argc, const char* argv[]) {
  ArgParser args;
  args.AddArg("output")
      .SetShortName('o')
      .SetNumArgs(1)
      .SetDescription("Prefix path for generated binaries.");
  args.AddArg("includes")
      .SetShortName('i')
      .SetVariableNumArgs()
      .SetDescription("Paths to search for includes in schemas.");
  args.AddArg("fbs")
      .SetShortName('f')
      .SetVariableNumArgs()
      .SetDescription("List of fbs schemas.")
      .SetRequired();
  args.AddArg("json")
      .SetShortName('j')
      .SetVariableNumArgs()
      .SetDescription(
          "List of entity jsons. Each will be saved to \"basename.bin\" in the "
          "output path.")
      .SetRequired();
  if (!args.Parse(argc, argv)) {
    auto& errors = args.GetErrors();
    for (auto& err : errors) {
      LOG(ERROR) << "Error: " << err;
    }
    LOG(ERROR) << args.GetUsage();
    return -1;
  }

  BlueprintFromJsonCompiler compiler;
  if (!ParseFbses(&compiler, args.GetValues("fbs"),
                  args.GetValues("includes"))) {
    return -1;
  }
  if (!ParseJsons(&compiler, args.GetValues("json"),
                  args.GetString("output"))) {
    return -1;
  }
  return 0;
}

}  // namespace tool
}  // namespace lull

int main(int argc, const char** argv) { return lull::tool::Run(argc, argv); }
