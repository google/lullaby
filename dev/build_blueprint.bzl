"""Build BlueprintDef binary assets from blueprint JSON files."""

load(
    "//tools/build_defs/jsonnet:jsonnet.bzl",
    "jsonnet_library",
    "jsonnet_to_json",
)
load(
    ":utils_common.bzl",
    "FLATC_DEFAULT_INCLUDES",
    "FLATC_DEFAULT_INCLUDE_PATHS",
)

def build_blueprint_bin(
        name,
        srcs,
        outdir = "",
        includes = [],
        include_paths = [],
        jsonnet_vars = {},
        jsonnet_deps = [],
        jsonnet_data = [],
        jsonnet_library_srcs = None,
        visibility = ["//visibility:public"]):
    """Generates BlueprintDef binaries from blueprint json (or jsonnet) files.

    The schema used to write the JSON matches the legacy EntityDef schema used in
    generate_entity_schema/build_entity, but we do not compile these in the same way.
    Instead, the components are each compiled individually and the finalized binaries
    are stored opaquely in BlueprintComponentDefs. Then, the BlueprintDef containing
    these components is compiled separately to produce the final flatbuffer binary
    for the whole blueprint.

    One key difference is that the "def_type" field of BlueprintComponentDef can
    contain the full namespaced component name, e.g. "lull.TransformDef". Also, while
    the JSON parser is now stricter, you can still just use jsonnet on all files to
    rely on its laxer rules, such as:
      * Trailing commas in objects/arrays
      * Unquoted object field names

    Unlike build_entity_bin(), merging of files with the same name is not supported.

    Args:
      name: name for the filegroup contain the set of generated .bin files
      srcs: list of blueprint .json files
      outdir: optional, folder in which to generate output .bin files
      includes: Optional, list of filegroups of schemas that the srcs depend on.
      include_paths: Optional, list of paths the includes files can be found in.
      jsonnet_vars: Optional, dictionary of variable names and values passed to
                    the jsonnet_to_json generation.
      jsonnet_deps: Optional, list of jsonnet libraries that will be used as
                    dependencies during jsonnet to json conversion.
      jsonnet_data: Optional, list of files that may be used during jsonnet to
                    json conversion (typically via importstr).
      jsonnet_library_srcs: Optional, list of srcs to generate a jsonnet library
                            which will be used as a dependency for converting
                            jsonnet files to json files.
      visibility: The visibility of the target. Defaults to public.
    """
    compiler_path = "//lullaby/tools/compile_blueprint_from_json"

    full_includes = depset(FLATC_DEFAULT_INCLUDES + includes).to_list()
    full_include_paths = depset(FLATC_DEFAULT_INCLUDE_PATHS + include_paths).to_list()

    if outdir:
        output_path = "%s/" % outdir
    else:
        output_path = ""

    # Generate a jsonnet library if a set of library srcs is provided.
    jsonnet_all_deps = list(jsonnet_deps)
    if jsonnet_library_srcs:
        jsonnet_library_name = "jsonnet_library_%s" % name
        jsonnet_library(
            name = jsonnet_library_name,
            srcs = jsonnet_library_srcs,
        )
        jsonnet_all_deps.append(":%s" % jsonnet_library_name)

    # Generate list of .json files from the |srcs|.  If |srcs| contains
    # .jsonnet files, convert them to .json files first.
    json_srcs = []
    for src in srcs:
        if src.endswith(".json"):
            json_srcs.append(src)
        elif src.endswith(".jsonnet"):
            label_suffix = (output_path + src).replace("/", "_").replace(":", "_")

            # When a json src file is included from somewhere else in the //depot,
            # appending the outdir to the src file generates an unparseable out target
            # name, which needs to be cleaned here.
            # eg. outdir///third_party/... -> outdir/third_party/...
            out = output_path + src.replace(":", "/").replace("//", "").rsplit(".", 1)[0] + ".json"
            json_srcs.append(out)

            jsonnet_to_json(
                name = "jsonnet_to_json_%s" % label_suffix,
                src = src,
                outs = [out],
                deps = jsonnet_all_deps,
                data = jsonnet_data,
                ext_strs = jsonnet_vars,
            )

    outs = []
    location_srcs = []
    for src in json_srcs:
        # Extract the basename from the target path.
        basename = src[:-len(".json")]
        if ":" in basename:
            basename = basename.split(":")[-1]
        basename = basename.split("/")[-1]
        out = "%s%s.bin" % (output_path, basename)
        outs.append(out)
        location = "$(location %s)" % (src)
        location_srcs.append(location)
    location_full_includes = ["$(locations %s)" % (s) for s in full_includes]

    # From the Build Encyclopedia, "If there is only one file name in outs, this expands to the
    # directory containing that file. If there are multiple files, this instead expands to the
    # package's root directory in the genfiles tree, even if all generated files belong to the
    # same subdirectory!"
    if len(outs) > 1 and outdir:
        output_path = "$(@D)/%s" % outdir
    else:
        output_path = "$(@D)"

    cmds = [
        "$(location %s)" % (compiler_path),
        "-o %s" % output_path,
        "-i ",
        " ".join(full_include_paths),
        "-f ",
        " ".join(location_full_includes),
        "-j ",
        " ".join(location_srcs),
    ]

    # Generate .bin files from .json files.
    native.genrule(
        name = "generate_blueprint_%s" % (name),
        srcs = json_srcs,
        outs = outs,
        tools = [compiler_path] + full_includes,
        cmd = " ".join(cmds),
    )

    # Create filegroup of generated .bin files.
    native.filegroup(
        name = name,
        srcs = outs,
        visibility = visibility,
    )
