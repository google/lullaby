"""Build Stategraph assets from source json(net) files."""

load(":utils_jsonnet.bzl", "jsonnet_to_json")
load(
    "@flatbuffers//:flatbuffers:build_defs.bzl",
    "flatbuffer_binaries",
)
load(
    ":utils_common.bzl",
    "FLATC_DEFAULT_INCLUDES",
    "FLATC_DEFAULT_INCLUDE_PATHS",
)

def build_stategraph(
        name,
        srcs,
        jsonnet_deps = [],
        jsonnet_data = [],
        visibility = None):
    """Generates Stategraph binaries from json/jsonnet files.

    Args:
      name: name for the filegroup containing the set of generated .stategraph
            files
      srcs: list of stategraph source files, either json or jsonnet
      jsonnet_deps: optional list of jsonnet depenendencies
      jsonnet_data: optional list of data files used during jsonnet conversion
      visibility: The visibility of the output. Defaults to None.
    """
    json_srcs = []
    jsonnet_srcs = []
    for src in srcs:
        if "jsonnet" in src:
            jsonnet_srcs.append(src)
            json_srcs.append(src.replace("jsonnet", "json"))
        else:
            json_srcs.append(src)

    jsonnet_to_json(
        name = "%s_jsonnet" % name,
        srcs = jsonnet_srcs,
        deps = jsonnet_deps + [
            "//data:jsonnet",
        ],
        data = jsonnet_data,
    )

    flatbuffer_binaries(
        name = name,
        include_paths = FLATC_DEFAULT_INCLUDE_PATHS,
        includes = depset(FLATC_DEFAULT_INCLUDES + [
            ":%s_jsonnet" % name,
        ]).to_list(),
        json_srcs = json_srcs,
        schema = "//:schemas/lull/animation_stategraph.fbs",
        strip_prefix = "",
        extension = ".stategraph",
        visibility = visibility,
    )
