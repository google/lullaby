"""Generates a cc_library from flatbuffer schemas."""

_FLATC_ARGS = [
    # Generate GetFullyQualifiedName() function for classes.
    "--gen-name-strings",
    # Generate code for mutating objects.
    "--gen-mutable",
    # Keep prefixes on include paths (ie. use full paths).
    "--keep-prefix",
    # Use `enum class` instead of `enum` for enums.
    "--scoped-enums",
    # Generate a native C++ object API.
    "--gen-object-api",
]

def flatbuffer_cc_library(
        name,
        srcs,
        deps = [],
        out_prefix = "",
        visibility = None):
    """Generates a cc_library from flatbuffer schemas.

    Args:
      name: The target name of the output cc_library.
      srcs: The source .fbs files; sent in order to the compiler.
      deps: Dependencies on other flatbuffer_cc_library targets.
      out_prefix: Specifies a prefix for all output headers, usually a directory.
      visibility: Visibility of the generated library and supporting targets.

    Outs:
      cc_library(name): library with sources and flatbuffers deps
    """

    # `deps` serve two purposes:
    # 1. Used as direct deps for the cc_library.
    # 2. Used to get the list of include files for flatc compiler. In this case, we
    #    assume the deps have also been built using this function. This allows us to
    #    create our own filegroups that we can then use as flatc includes.
    deps = depset(deps).to_list()

    # Build a filegroup for the inputs. We'll need this to allow flatc to include
    # these files when there's a dependecy on this target.
    native.filegroup(
        name = "%s_files" % name,
        srcs = srcs,
        visibility = visibility,
    )

    includes = ["%s_files" % x for x in deps]

    # Generate the flatbuffer header files using the flatc compiler.
    flatc_outs = [out_prefix + s.split("/")[-1].replace(".fbs", "_generated.h") for s in srcs]
    _flatbuffer_library(
        name = "%s_hdrs" % name,
        srcs = srcs,
        outs = flatc_outs,
        flatc_args = _FLATC_ARGS,
        includes = includes,
        language_flag = "-c",
        out_prefix = out_prefix,
    )

    # Build the cc_library using the generated headers.
    native.cc_library(
        name = name,
        hdrs = [":%s_hdrs" % name],
        deps = deps + ["@flatbuffers//:flatbuffers"],
        visibility = visibility,
    )

def _flatbuffer_library(
        name,
        srcs,
        outs,
        language_flag,
        out_prefix = "",
        includes = [],
        flatc_args = []):
    """Generates code files for reading/writing the given flatbuffers in the requested language using the public compiler.

    Args:
      name: Rule name.
      srcs: Source .fbs files. Sent in order to the compiler.
      outs: Output files from flatc.
      language_flag: Target language flag. One of [-c, -j, -js].
      out_prefix: Prepend this path to the front of all generated files except on
          single source targets. Usually is a directory name.
      includes: Optional, list of filegroups of schemas that the srcs depend on.
      flatc_args: Optional, list of additional arguments to pass to flatc.
    Outs:
      filegroup(name): all generated source files.
    """
    flatc_path = "@flatbuffers//:flatc"
    include_paths = ["./", "$(GENDIR)", "$(BINDIR)"]
    include_paths_cmd = ["-I %s" % s for s in include_paths]

    # '$(@D)' when given a single source target will give the appropriate
    # directory. Appending 'out_prefix' is only necessary when given a build
    # target with multiple sources.
    output_directory = (
        ("-o $(@D)/%s" % out_prefix) if len(srcs) > 1 else "-o $(@D)"
    )
    genrule_cmd = " ".join([
        "SRCS=($(SRCS));",
        "for f in $${SRCS[@]:0:%s}; do" % len(srcs),
        "$(location %s)" % flatc_path,
        " ".join(include_paths_cmd),
        " ".join(flatc_args),
        language_flag,
        output_directory,
        "$$f;",
        "done",
    ])
    native.genrule(
        name = name,
        srcs = srcs + includes,
        outs = outs,
        tools = [flatc_path],
        cmd = genrule_cmd,
        message = "Generating flatbuffer files for %s:" % name,
    )
