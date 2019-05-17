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
#include "lullaby/viewer/src/viewer.h"


static int RunViewer(int argc, const char** argv) {
  lull::ArgParser args;
  args.AddArg("importdir").SetNumArgs(1).SetDescription("Asset import path.");
  args.AddArg("json").SetNumArgs(1).SetDescription(
      "Json(net) file containing an entity to create.");

  // Parse the command-line arguments.
  if (!args.Parse(argc, argv)) {
    auto& errors = args.GetErrors();
    for (auto& err : errors) {
      std::cout << "Error: " << err << std::endl;
    }
    std::cout << args.GetUsage() << std::endl;
    return -1;
  }

  lull::tool::Viewer::InitParams params;
  params.width = 1280;
  params.height = 720;
  params.label = "Lullaby Viewer";

  lull::tool::Viewer viewer;
  viewer.Initialize(params);

  // Use any command line args.
  const std::string import_dir(args.GetString("importdir"));
  if (!import_dir.empty()) {
    viewer.ImportDirectory(import_dir);
  }
  const std::string json(args.GetString("json"));
  if (!json.empty()) {
    viewer.CreateEntity(json);
  }

  while (!viewer.ShouldQuit()) {
    viewer.Update();
  }
  viewer.Shutdown();
  return viewer.GetExitCode();
}

int main(int argc, const char** argv) { return RunViewer(argc, argv); }
