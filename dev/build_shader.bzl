"""Build Shader assets from source glslv/glslf files."""

def build_shader(
        name,
        srcs,
        out,
        version = "",
        defines = [],
        include_deps = [],
        strip_prefix = "",
        build_multiview = False,
        visibility = ["//visibility:public"]):
    """Generates an fplshader file based on the given arguments and returns a Fileset usable as a data dependency in a binary target to set up the linking consistent with FPL asset layout.

    Args:
      name: string, desired Fileset target name.
      srcs: dictionary, stages and filenames of shader sources.
      out: string, output filename.
      version: optional string, version string to use during compilation.
      defines: optional array of strings, definitions to include in each shader.
      include_deps: optional array of possibly included header files. These will
                    be made available on the machine executing the genrule.
      strip_prefix: optional string, will be stripped from all input file paths
                    in output file generation. All subdirectories after
                    strip_prefix will be retained.
      build_multiview: optional bool, True if multiview-capable versions of the
                       shader should be built as well. The multiview-capable
                       shader will be placed in "multiview/<out>" and have the
                       following modifications:
                       - version set to "300 es"
                       - "MULTIVIEW" added to defines
                       Two additional BUILD targets are generated as well:
                       "<name>_only_multiview" provides only the multiview version
                       and "<name>_with_multiview" provides both.
      visibility: optional array will be set as the visiblity argument for the
                  fileset
    """

    _build_shader_internal(
        name,
        srcs,
        out,
        version,
        defines,
        include_deps,
        strip_prefix,
    )

    native.filegroup(
        name = name,
        srcs = [out],
        visibility = visibility,
    )

    if (build_multiview):
        multiview_version = "300 es"
        multiview_defines = defines + ["MULTIVIEW"]
        multiview_out = "multiview/" + out

        _build_shader_internal(
            name,
            srcs,
            multiview_out,
            multiview_version,
            multiview_defines,
            include_deps,
            strip_prefix,
        )
        native.filegroup(
            name = "%s_only_multiview" % name,
            srcs = [multiview_out],
            visibility = visibility,
        )
        native.filegroup(
            name = "%s_with_multiview" % name,
            srcs = [out, multiview_out],
            visibility = visibility,
        )

def _build_shader_internal(
        name,
        srcs,
        out,
        version = "",
        defines = [],
        include_deps = [],
        strip_prefix = ""):
    """Internal utility for creating shader genrules.

    Args:
      name: string, desired Fileset target name.
      srcs: dictionary, stages and filenames of shader sources.
      out: string, output filename.
      version: optional string, version string to use during compilation.
      defines: optional array of strings, definitions to include in each shader.
      include_deps: optional array of possibly included header files. These will
                    be made available on the machine executing the genrule.
      strip_prefix: optional string, will be stripped from all input file paths
                    in output file generation. All subdirectories after
                    strip_prefix will be retained.
    """

    shader_pipeline_path = "//third_party/fplbase/shader_pipeline"
    vertex = srcs["vertex"]
    fragment = srcs["fragment"]

    command_array = [
        "$(location %s)" % shader_pipeline_path,
        "-vs $(location %s)" % vertex,
        "-fs $(location %s)" % fragment,
    ]

    for define in defines:
        command_array += ["-d", define]

    if version:
        command_array += ["--version", "\"%s\"" % version]

    command_array += ["$@"]

    if strip_prefix:
        out = out.split(strip_prefix + "/")[-1]

    native.genrule(
        name = "%s_%s_genrule" % (name, out),
        srcs = [fragment, vertex] + include_deps,
        outs = [out],
        tools = [shader_pipeline_path],
        cmd = " ".join(command_array),
        message = "Calling shader_pipeline for target %s" % (name),
    )

def build_lullaby_shader(
        name,
        srcs_vertex,
        srcs_fragment,
        out,
        include_deps = [],
        jsonnet_deps = [],
        strip_prefix = "",
        visibility = ["//visibility:public"]):
    """Generates a lull shader and returns a Fileset usable as a data dependency.

    Args:
      name: string, desired Fileset target name.
      srcs_vertex: array of vertex snippet jsonnet files.
      srcs_fragment: array of fragment snippet jsonnet files.
      out: string, output filename.
      include_deps: optional array of possibly included header files. These will
                    be made available on the machine executing the genrule.
      jsonnet_deps: optional array of possibly included jsonnet files used by the
                    shaders.
      strip_prefix: optional string, will be stripped from all input file paths
                    in output file generation. All subdirectories after
                    strip_prefix will be retained.
      visibility: optional array will be set as the visiblity argument for the
                  fileset
    """

    shader_schema_path = "schemas/lull/shader_def.fbs"
    shader_compiler_path = "//lullaby/tools/shader_pipeline"
    fbs_schema_includes = ["//:fbs_includes"]

    command_array = [
        "$(location %s)" % shader_compiler_path,
        " ".join(["--vertex_sources $(location %s)" % v for v in srcs_vertex]),
        " ".join(["--fragment_sources $(location %s)" % f for f in srcs_fragment]),
        "--schema %s" % shader_schema_path,
        "--out $(location %s)" % out,
        "$@",
    ]

    if strip_prefix:
        out = out.split(strip_prefix + "/")[-1]

    native.genrule(
        name = "%s_%s_genrule" % (name, out),
        srcs = srcs_vertex + srcs_fragment + include_deps +
               fbs_schema_includes + jsonnet_deps +
               ["//data:jsonnet_files"],
        outs = [out],
        tools = [shader_compiler_path],
        cmd = " ".join(command_array),
        message = "Calling shader_pipeline for target %s" % name,
    )

    native.filegroup(name = name, srcs = [out], visibility = visibility)

def build_matc(
        name,
        src,
        out,
        visibility = ["//visibility:public"]):
    """Generates a filament matc file.

    Args:
      name: string, desired Fileset target name.
      src: the source filament material json file.
      out: string, output filename.
      visibility: optional array will be set as the visiblity argument for the
                  fileset
    """

    matc = "@filament//:filament:matc"

    command_array = [
        "$(location %s)" % matc,
        "$(location %s)" % src,
        "--output $(location %s)" % out,
    ]

    native.genrule(
        name = "%s_%s_genrule" % (name, out),
        srcs = [src],
        outs = [out],
        tools = [matc],
        cmd = " ".join(command_array),
        message = "Calling shader_pipeline for target %s" % name,
    )

    native.filegroup(name = name, srcs = [out], visibility = visibility)
