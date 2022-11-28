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

#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "redux/modules/math/constants.h"
#include "redux/tools/anim_pipeline/anim_pipeline.h"
#include "redux/tools/anim_pipeline/animation.h"
#include "redux/tools/common/file_utils.h"
#include "redux/tools/common/flatbuffer_utils.h"
#include "redux/tools/common/log_utils.h"

ABSL_FLAG(std::string, input, "", "Input asset.");
ABSL_FLAG(std::string, output, "", "Exported rxanim file.");
ABSL_FLAG(std::string, logfile, "", "Log file for export information.");

ABSL_FLAG(float, cm_per_unit, 0.0f,
          "Defines the unit the pipeline expects positions to be in compared "
          "to centimeters (the standard unit for FBX). For example, 100.0 "
          "would be in contexts where world units are measured in meters, and "
          "2.54 would be for inches. Keep at 0 to leave asset units as-is.");
ABSL_FLAG(std::string, axis, "",
          "Specified which axes are up, front, and left.");
ABSL_FLAG(float, scale_multiplier, 1.0f,
          "Overall scale multiplier applied to the entire animation.");
ABSL_FLAG(float, translation_tolerance, 0.01f,
          "Amount output translate curves can deviate, in scene's distance "
          "units.");
ABSL_FLAG(float, quaternion_tolerance, 0.001f,
          "Amount output quaternion curves can deviate, unitless.");
ABSL_FLAG(float, scale_tolerance, 0.005f,
          "Amount output scale curves can deviate, unitless.");
ABSL_FLAG(float, angle_tolerance, 0.5f,
          "Amount derivative, converted to an angle in x/y, can deviate, in "
          "degrees.");
ABSL_FLAG(bool, preserve_start_time, false,
          "Allow animations to start at a non-zero time.");
ABSL_FLAG(bool, stagger_end_times, false,
          "Allow each channel to end at a different time");

namespace redux::tool {

AnimationPtr ImportAsset(std::string_view uri, const ImportOptions& opts);
AnimationPtr ImportFbx(std::string_view uri, const ImportOptions& opts);

static int RunAnimPipeline() {
  const std::string input = absl::GetFlag(FLAGS_input);
  CHECK(!input.empty())  << "Must specify 'input' argument.";

  const std::string output = absl::GetFlag(FLAGS_output);
  CHECK(!output.empty()) << "Must specify output file.";

  LoggerOptions log_opts;
  log_opts.logfile = absl::GetFlag(FLAGS_logfile);
  Logger log(log_opts);

  AnimPipeline pipeline(log);
  pipeline.RegisterImporter(ImportFbx, ".fbx");
  pipeline.RegisterImporter(ImportAsset, ".dae");
  pipeline.RegisterImporter(ImportAsset, ".obj");
  pipeline.RegisterImporter(ImportAsset, ".gltf");
  pipeline.RegisterImporter(ImportAsset, ".glb");

  DataContainer data;
  if (!input.empty()) {
    ImportOptions opts;
    opts.cm_per_unit = absl::GetFlag(FLAGS_cm_per_unit);
    opts.axis_system = ReadAxisSystem(absl::GetFlag(FLAGS_axis));
    opts.scale_multiplier = absl::GetFlag(FLAGS_scale_multiplier);
    opts.preserve_start_time = absl::GetFlag(FLAGS_preserve_start_time);
    opts.stagger_end_times = absl::GetFlag(FLAGS_stagger_end_times);
    opts.tolerances.translate = absl::GetFlag(FLAGS_translation_tolerance);
    opts.tolerances.quaternion = absl::GetFlag(FLAGS_quaternion_tolerance);
    opts.tolerances.scale = absl::GetFlag(FLAGS_scale_tolerance);
    opts.tolerances.derivative_angle =
        absl::GetFlag(FLAGS_angle_tolerance) * kDegreesToRadians;
    data = pipeline.Build(input, opts);
  }

  CHECK(SaveFile(data.GetBytes(), data.GetNumBytes(), output.c_str(), true))
      << "Failed to save to file: " << output;
  return 0;
}

}  // namespace redux::tool

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  return redux::tool::RunAnimPipeline();
}
