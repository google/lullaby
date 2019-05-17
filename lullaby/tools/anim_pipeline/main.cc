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
#include "lullaby/util/math.h"
#include "lullaby/tools/anim_pipeline/anim_pipeline.h"
#include "lullaby/tools/anim_pipeline/import_options.h"
#include "lullaby/tools/common/file_utils.h"

namespace lull {
namespace tool {

std::vector<Animation> ImportFbx(const std::string& filname,
                                 const ImportOptions& source);
std::vector<Animation> ImportAsset(const std::string& filname,
                                   const ImportOptions& source);
int SaveAnim(const AnimPipeline::ExportedAnimation& exported_anim,
             const std::string& out_fullpath, bool gnuplot);

int Run(int argc, const char** argv) {
  ArgParser args;
  // clang-format off
  // Input/output flags.
  args.AddArg("input")
      .SetNumArgs(1)
      .SetRequired()
      .SetDescription("Asset file to process.");
  args.AddArg("output")
      .SetNumArgs(1)
      .SetRequired()
      .SetDescription("Anim file to save.");
  args.AddArg("outdir")
      .SetNumArgs(1)
      .SetRequired()
      .SetDescription("Location (path) to save file.");
  args.AddArg("ext")
      .SetNumArgs(1)
      .SetDefault("motiveanim")
      .SetDescription("Extension to use for the output file. Defaults to "
                      "'motiveanim'.");

  // Tolerance flags.
  args.AddArg("scale")
      .SetNumArgs(1)
      .SetDescription("Maximum deviation of output scale curves from input "
                      "scale curves; unitless. Default is 0.005.");
  args.AddArg("rotate")
      .SetNumArgs(1)
      .SetDescription("Maximum deviation of output Euler rotation curves from "
                      "input rotation curves; in degrees. Default is 0.5 "
                      "degrees.");
  args.AddArg("translate")
      .SetNumArgs(1)
      .SetDescription("Maximum deviation of output translation curves from "
                      "input translation curves; in scene units. Default is "
                      "0.01 scene units.");
  args.AddArg("quaternion")
      .SetNumArgs(1)
      .SetDescription("Maximum deviation of output quaternion rotation curves "
                      "from input rotation curves; unitless. Default is "
                      "0.001.");
  args.AddArg("angle")
      .SetNumArgs(1)
      .SetDescription("Maximum deviation of curve derivatives from input curve "
                      "derivatives as an angle in the X/Y plane (e.g. deriv 1 "
                      "=> 45 degrees); in degrees. Default is 0.5 degrees.");

  // Other flags.
  args.AddArg("preserve_start_time")
      .SetDescription("Start the animation at the same time as in the source. "
                      "By default, the animation is shifted such that its "
                      "start time is zero.");
  args.AddArg("stagger_end_times")
      .SetDescription("Allow every channel to end at its authored time instead "
                      "of adding extra spline nodes to plum-up every channel. "
                      "This may cause strage behavior with animations that "
                      "repeat, since the shorter channels will start to repeat "
                      "before the longer ones.");
  args.AddArg("nouniformscale")
      .SetDescription("Prevents scale X/Y/Z channels from being collapsed into "
                      "a single uniform scale channel even if they have "
                      "identical curves.");
  args.AddArg("gnuplot")
      .SetDescription("Writes out animation channels in gnuplot format for "
                      "visualization. Files are saved to '<outdir>/*.gnuplot'; "
                      "each file is named after a single bone and contains "
                      "all the channel curves for that bone. To plot, copy "
                      "and paste the shell command from the file's header.");
  args.AddArg("sqt")
      .SetDescription("Bakes the output curve data into curves representing "
                      "scale, quaternion rotation, and translation curves. "
                      "Blending between SQT and non-SQT animations is "
                      "unsupported.");

  // Deprecated flags included for compatibility with the Motive anim pipeline.
  args.AddArg("norepeat").SetDeprecated();
  // clang-format on

  // Parse the command-line arguments.
  if (!args.Parse(argc, argv)) {
    auto& errors = args.GetErrors();
    for (auto& err : errors) {
      std::cout << "Error: " << err << std::endl;
    }
    std::cout << args.GetUsage() << std::endl;
    return -1;
  }

  AnimPipeline pipeline;
  pipeline.RegisterImporter(ImportFbx, ".fbx");
  pipeline.RegisterImporter(ImportAsset, ".dae");
  pipeline.RegisterImporter(ImportAsset, ".gltf");
  pipeline.RegisterImporter(ImportAsset, ".obj");

  ImportOptions options;

  if (args.IsSet("scale")) {
    options.tolerances.scale = args.GetFloat("scale");
  }
  if (args.IsSet("rotate")) {
    options.tolerances.rotate = args.GetFloat("rotate") * kDegreesToRadians;
  }
  if (args.IsSet("translate")) {
    options.tolerances.translate = args.GetFloat("translate");
  }
  if (args.IsSet("quaternion")) {
    options.tolerances.quaternion = args.GetFloat("quaternion");
  }
  if (args.IsSet("angle")) {
    options.tolerances.derivative_angle =
        args.GetFloat("angle") * kDegreesToRadians;
  }

  if (args.IsSet("preserve_start_time")) {
    options.preserve_start_time = true;
  }

  if (args.IsSet("stagger_end_times")) {
    options.stagger_end_times = true;
  }

  if (args.IsSet("nouniformscale")) {
    options.no_uniform_scale = true;
  }

  if (args.IsSet("sqt")) {
    options.sqt_animations = true;
  }

  if (!pipeline.Import(std::string(args.GetString("input")), options)) {
    return -1;
  }

  // Create the output folder if necessary.
  const std::string out_dir(args.GetString("outdir"));
  if (!CreateFolder(out_dir.c_str())) {
    LOG(ERROR) << "Could not create directory: " << out_dir;
    return -1;
  }
  const std::string anim_name = RemoveDirectoryAndExtensionFromFilename(
      std::string(args.GetString("output")));
  const std::string ext(args.GetString("ext"));

  /// Loop through resulting animations.  This will always be 1, unless
  /// options.import_clips is true.
  if (options.import_clips) {
    for (int i = 0; i < pipeline.GetExportCount(); i++) {
      const AnimPipeline::ExportedAnimation& exported_anim =
          pipeline.GetExport(i);
      const std::string outfile =
          out_dir + "/" + anim_name + "::" + exported_anim.name + "." + ext;
      if (SaveAnim(exported_anim, outfile, args.IsSet("gnuplot")) < 0) {
        return -1;
      }
    }
  } else {
    const std::string outfile = out_dir + "/" + anim_name + "." + ext;
    if (SaveAnim(pipeline.GetExport(0), outfile, args.IsSet("gnuplot")) < 0) {
      return -1;
    }
  }

  return 0;
}

int SaveAnim(const AnimPipeline::ExportedAnimation& exported_anim,
             const std::string& out_fullpath, bool gnuplot) {
  const ByteArray& flatbuffer = exported_anim.motive_anim;
  if (!SaveFile(flatbuffer.data(), flatbuffer.size(), out_fullpath.c_str(),
                true)) {
    LOG(ERROR) << "Unable to save animation.";
    return -1;
  }

  if (gnuplot) {
    const std::string out_dir = GetDirectoryFromFilename(out_fullpath);
    const std::string anim_name =
        RemoveDirectoryAndExtensionFromFilename(out_fullpath);
    const std::string gnuplot_dir = out_dir + "/" + anim_name + ".gnuplot";
    if (exported_anim.anim->GnuplotAllChannels(gnuplot_dir)) {
      LOG(INFO) << "Saved gnuplot files to '" << gnuplot_dir << "'.\n";
    } else {
      LOG(ERROR) << "Unable to save gnuplot files.";
    }
  }
  return 0;
}

}  // namespace tool
}  // namespace lull

int main(int argc, const char** argv) { return lull::tool::Run(argc, argv); }
