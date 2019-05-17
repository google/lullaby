"""Build Animations assets from source 3d asset files or json files."""

load("@motive//:motive:build_defs.bzl", "motive_anim_assets")

def build_animation(
        name,
        srcs,
        additional_anim_pipeline_args = "--norepeat",
        strip_prefix = "",
        visibility = ["//visibility:public"]):
    """Generates a motiveanim binary from source files.

    Args:
      name: name for the filegroup contain the set of generated motiveanim files
      srcs: list of 3d animation fbx files
      additional_anim_pipeline_args: additional arguments to pass to the animation
                                     pipeline. Run the pipeline directly to see a
                                     list of all possible arguments.
      strip_prefix: optional string, will be stripped from all input file paths
                    in output file generation. All subdirectories after
                    strip_prefix will be retained.
      visibility: The visibility of the entity target. Defaults to public.
    """
    build_lull_animation(
        name = name,
        additional_anim_pipeline_args = additional_anim_pipeline_args,
        srcs = srcs,
        strip_prefix = strip_prefix,
        visibility = visibility,
    )

def build_lull_animation(
        name,
        srcs,
        ext = "motiveanim",
        extra_srcs = [],
        additional_anim_pipeline_args = "",
        strip_prefix = "",
        visibility = ["//visibility:public"]):
    """Generates a motiveanim binary from source files.

    Args:
      name: Name for the filegroup containing the set of generated motiveanim files.
      srcs: List of 3d animation files.
      ext: Optional string, the extension for the output animation files.
      extra_srcs: Additional files required to generate animations that should not
                  have their own generated outputs.
      additional_anim_pipeline_args: Optional string, additional arguments to be passed
                                     to anim_pipeline.
      strip_prefix: Optional string, will be stripped from all input file paths
                    in output file generation. All subdirectories after
                    strip_prefix will be retained.
      visibility: The visibility of the output filegroup. Defaults to public.
    """
    tool = "//lullaby/tools/anim_pipeline"
    outs = []

    for src in srcs:
        # Replace source file extension with output file extension
        # TODO: For now, apply the strip prefix every time because the old
        # motive_anim_assets used to do this. Fixing it will require updating clients.
        out = ".".join(src.split(strip_prefix + "/")[-1].split(".")[:-1]) + "." + ext

        cmd = []
        cmd += ["$(location %s)" % tool]
        cmd += [additional_anim_pipeline_args]
        cmd += ["--input $(location %s)" % src]
        if ext:
            cmd += ["--ext %s" % ext]
        cmd += ["--outdir $(@D)"]
        cmd += ["--output $@"]
        genrule_srcs = [src]
        if extra_srcs:
            genrule_srcs += extra_srcs
        native.genrule(
            name = "build_%s" % out,
            srcs = genrule_srcs,
            tools = [tool],
            outs = [out],
            cmd = " ".join(cmd),
        )
        outs += [out]

    native.filegroup(name = name, srcs = outs, visibility = visibility)
