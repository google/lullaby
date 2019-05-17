"""Build/convert image files to be used as textures."""

load(
    "//third_party/fplbase:build_defs.bzl",
    "fpl_png_assets",
)

def build_etc_texture(
        name,
        srcs = [],
        filenames = [],
        filegroups = [],
        strip_prefix = "src/rawassets",
        fmt = "RGB8",
        effort = 60,
        create_mips = False):
    """Given a list of input .png files, converts them into .ktx output files and then generates a Fileset() of the given name.

    If converting local files, list them in srcs.

    If converting files from other projects, list them in filenames and include
    the appropriate filegroup build rules in filegroups.

    See //third_party/etc2comp/EtcTool/EtcTool.cpp for command line usage.

    This uses the etc2comp tool, access to which is whitelisted.

    Args:
      name: string, name of the Fileset to generate containing the output
        converted images.
      srcs: list of strings, relative path to the local image to be converted
      filenames: list of strings, path to the image to be converted.
        do not include a "//" at the start of the filename.
      filegroups: list of strings, build rules for filegroups that contain the
        filenames. Every entry in filenames must be in one of the filegroups
        listed here.  Changing any of these groups will force all filenames to be
        regenerated.
      strip_prefix: optional string, will be stripped from all input file paths
        in output file generation. All subdirectories after strip_prefix will be
        retained.
      fmt: optional string.  One of:
        ETC1, RGB8, SRGB8, RGBA8, SRGB8, RGB8A1, SRGB8A1, R11.
      effort: optional integer, how much additional effort to apply during
        encoding, from 0 to 100 (best).  From the release announcement blog,
        'setting the effort level to 60% results in roughly the same visual
        quality as Mali (in slow mode), but is significantly faster.'
      create_mips: optional bool, determines if the tool generates mip levels.
    """
    if filenames or filegroups:
        if not filenames or not filegroups:
            fail("If either filenames or filegroups are specified, both arguments" +
                 " must be used.")

    outs = []
    for src in srcs:
        _out = ""
        if strip_prefix:
            _out = src.replace(".png", ".ktx").split(strip_prefix + "/")[-1]
        else:
            _out = src.replace(".png", ".ktx")
        outs += [_out]
        _lullaby_etc_internal(name, _out, src, [src], fmt, effort, create_mips)

    for src in filenames:
        _out = ""
        if strip_prefix:
            _out = src.replace(".png", ".ktx").split(strip_prefix + "/")[-1]
        else:
            _out = src.replace(".png", ".ktx")
        outs += [_out]
        _lullaby_etc_internal(name, _out, src, filegroups, fmt, effort, create_mips)

    native.filegroup(name = name, srcs = outs)

def _lullaby_etc_internal(name, out, src, srcs, fmt, effort, create_mips):
    """Internal utility for creating etc2comp genrule calls.

    Args:
      name: string, name of the build rule.
      out: string, file to be created.
      src: string, png file to be converted.
      srcs: list of strings, either a list of files that includes src, or a list
        of filegroups that includes src
      fmt: string, texture format to use during encoding.
      effort: integer, encoding quality.
      create_mips: bool.
    """
    etctool_path = "//third_party/etc2comp:etctool"

    num_jobs = 4
    max_levels = 1000 if create_mips else 1

    native.genrule(
        name = "%s_%s" % (name, src),
        srcs = srcs,
        outs = [out],
        tools = [etctool_path],
        cmd = " ".join([
            "$(location %s)" % (etctool_path),
            "%s" % src,
            "-effort %s" % effort,
            "-format %s" % fmt,
            "-j %d" % num_jobs,
            "-mipmaps %d" % max_levels,
            "-output $@",
        ]),
        message = "Generating ktx file for %s" % src,
    )

def build_cubemap_texture(
        name,
        out,
        srcs):
    """Utility to convert 6 separate images into a single cubemap.

    Lullaby reads in cubemaps as single images that contain the six faces stacked
    on top of each other in GL order: +x, -x, +y, -y, +z, -z. This build rule
    takes the faces as separate images and stacks them into this format.

    This requires ImageMagick's convert tool, access to which is whitelisted.

    Args:
      name: string, name of the build rule.
      out: string, file to be created.
      srcs: dict of strings, paths to the face images using the keys:
            left = -x, right = +x, top = +y, bottom = -y, back = +z, front = -z.
    """

    if sorted(srcs.keys()) != ["back", "bottom", "front", "left", "right", "top"]:
        fail("Expected 6 faces")

    convert_path = "//third_party/ImageMagick:convert"

    ext = out.split(".")[-1]

    stacked_image = out.replace(ext, "png")

    native.genrule(
        name = name + "_stack",
        srcs = srcs.values(),
        outs = [stacked_image],
        tools = [convert_path],
        cmd = " ".join([
            "$(location %s)" % (convert_path),
            "-append",
            srcs["right"],
            srcs["left"],
            srcs["top"],
            srcs["bottom"],
            srcs["back"],
            srcs["front"],
            "$@",
        ]),
        message = "Stacking cubemap faces into %s" % stacked_image,
    )

    if ext == "webp":
        fpl_png_assets(
            name = name,
            srcs = [stacked_image],
            # Use a lossless conversion so faces don't bleed.
            lossless = True,
        )
    else:
        fail("Unrecognized output extension: " + ext)
