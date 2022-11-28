"""Generates an rxanim (redux animation) file from source assets."""

def build_anim(
        name,
        src,
        out,
        extra_args = [],
        include_deps = [],
        strip_prefix = "",
        generate_logfile = True,
        visibility = ["//visibility:public"]):
    """Generates a redux anim and returns a Fileset usable as a data dependency.

    Args:
      name: string, desired Fileset target name.
      src: input jsonnet file.
      out: string, output filename.
      extra_args: additional arguments to pass into model_pipeline.
      include_deps: optional array of possibly included header files. These will
                    be made available on the machine executing the genrule.
      strip_prefix: optional string, will be stripped from all input file paths
                    in output file generation. All subdirectories after
                    strip_prefix will be retained.
      generate_logfile: generates an additional output file containing useful information
      visibility: the visiblity argument for the fileset
    """
    anim_pipeline = "//redux/tools/anim_pipeline"
    fbs_schema_includes = [
        "//redux/modules/graphics:graphics_enums_fbs",
        "//redux/data/asset_defs:anim_asset_def_fbs",
    ]

    argv = [
        "$(location %s)" % anim_pipeline,
        "--input $(location %s)" % src,
        "--output $(location %s)" % out,
    ]

    if extra_args:
        argv += extra_args

    if strip_prefix:
        out = out.split(strip_prefix + "/")[-1]

    outs = [out]
    if generate_logfile:
        logfile = out + ".log"
        outs.append(logfile)
        argv.append("--logfile $(location %s)" % logfile)

    native.genrule(
        name = "%s_genrule" % (name),
        srcs = [src] + include_deps + fbs_schema_includes,
        outs = outs,
        tools = [anim_pipeline],
        cmd = " ".join(argv),
        message = "Calling anim_pipeline for target %s" % name,
    )

    native.filegroup(
        name = name,
        srcs = outs,
        visibility = visibility,
    )
