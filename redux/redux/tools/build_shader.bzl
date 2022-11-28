"""Generates an rxshader (redux shader) file from source data."""

def build_shader(
        name,
        src,
        out,
        include_deps = [],
        strip_prefix = "",
        visibility = ["//visibility:public"]):
    """Generates a redux shader and returns a Fileset usable as a data dependency.

    Args:
      name: string, desired Fileset target name.
      src: input jsonnet file.
      out: string, output filename.
      include_deps: optional array of possibly included header files. These will
                    be made available on the machine executing the genrule.
      strip_prefix: optional string, will be stripped from all input file paths
                    in output file generation. All subdirectories after
                    strip_prefix will be retained.
      visibility: the visiblity argument for the fileset
    """
    shader_pipeline = "//redux/tools/shader_pipeline"
    fbs_schema_includes = [
        "//redux/modules/graphics:graphics_enums_fbs",
        "//redux/data/asset_defs:shader_asset_def_fbs",
    ]

    argv = [
        "$(location %s)" % shader_pipeline,
        "--src $(location %s)" % src,
        "--out $(location %s)" % out,
        "$@",
    ]

    if strip_prefix:
        out = out.split(strip_prefix + "/")[-1]

    native.genrule(
        name = "%s_genrule" % (name),
        srcs = [src] + include_deps + fbs_schema_includes,
        outs = [out],
        tools = [shader_pipeline],
        cmd = " ".join(argv),
        message = "Calling shader_pipeline for target %s" % name,
    )

    native.filegroup(
        name = name,
        srcs = [out],
        visibility = visibility,
    )
