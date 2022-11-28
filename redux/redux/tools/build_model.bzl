"""Generates an rxmodel (redux model) file from source assets."""

def build_model(
        name,
        src,
        out,
        extra_args = [],
        include_deps = [],
        strip_prefix = "",
        generate_logfile = True,
        visibility = ["//visibility:public"]):
    """Generates a redux model and returns a Fileset usable as a data dependency.

    Args:
      name: string, the target Fileset to generate.
      src: string, input asset file.
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
    model_pipeline = "//redux/tools/model_pipeline"
    fbs_schema_includes = [
        "//redux/modules/graphics:graphics_enums_fbs",
        "//redux/data/asset_defs:model_asset_def_fbs",
    ]

    argv = [
        "$(location %s)" % model_pipeline,
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
        srcs = [src] + fbs_schema_includes + include_deps,
        outs = outs,
        tools = [model_pipeline],
        cmd = " ".join(argv),
        message = "Calling model_pipeline for target %s" % name,
    )

    native.filegroup(
        name = name,
        srcs = outs,
        visibility = visibility,
    )
