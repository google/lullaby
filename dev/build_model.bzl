"""Build Model assets from source 3d asset files like fbx, obj, gltf, etc."""

load("//third_party/fplbase:build_defs.bzl", "fpl_png_assets")

def build_model(
        name,
        srcs,
        textures = [],
        extra_srcs = [],
        strip_prefix = "",
        attrib = None,
        ext = "lullmodel",
        visibility = ["//visibility:public"]):
    """Generates a fplmesh binary from source files.

    Args:
      name: name for the filegroup contain the set of generated motiveanim files
      srcs: list of 3d asset files
      textures: list of all textures associated with the model
      strip_prefix: optional string, will be stripped from all input file paths
                    in output file generation. All subdirectories after
                    strip_prefix will be retained.
      attrib: a string specifying which vertex attributes should be output.
      ext: A file extension to replace each input file extension with for output.
      visibility: The visibility of the entity target. Defaults to public.
    """
    tool = "//lullaby/tools/model_pipeline"

    outs = []
    textures_name = "%s_textures" % name
    if textures:
        fpl_png_assets(
            name = textures_name,
            srcs = textures,
            strip_prefix = strip_prefix,
            webp_quality = 90,
        )

    for src in srcs:
        # Replace source file extension with output file extension
        out = ".".join(src.split(".")[:-1]) + "." + ext
        if strip_prefix:
            out = out.split(strip_prefix + "/")[-1]

        cmd = []
        if textures:
            cmd += ["textures=\"\";"]
            cmd += ["for f in $(locations %s); do" % (":" + textures_name)]
            cmd += ["  textures+=$$f\";\";"]
            cmd += ["done;"]
        cmd += ["$(location %s)" % tool]
        cmd += ["--input $(location %s)" % src]
        cmd += ["--schema schemas/lull/model_pipeline_def.fbs"]
        if attrib:
            cmd += ["--attrib %s" % attrib]
        if ext:
            cmd += ["--ext %s" % ext]
        cmd += ["--outdir $(@D)"]
        cmd += ["--output $@"]
        genrule_srcs = [src]
        if textures:
            cmd += ["--textures \"$$textures\";"]
            genrule_srcs += [":" + textures_name]
        if extra_srcs:
            genrule_srcs += extra_srcs

        native.genrule(
            name = "build_%s" % out,
            srcs = genrule_srcs,
            tools = ["//:model_schema"] + [tool],
            outs = [out],
            cmd = " ".join(cmd),
        )
        outs += [out]

    native.filegroup(name = name, srcs = outs, visibility = visibility)
