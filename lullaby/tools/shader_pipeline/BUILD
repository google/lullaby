# Pipeline tool for building LullShader files.

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "shader_pipeline_lib",
    srcs = [
        "build_shader.cc",
        "process_shader_source.cc",
    ],
    hdrs = [
        "build_shader.h",
        "process_shader_source.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@flatbuffers//:flatc_library",
        "@jsonnet//core:libjsonnet",
        "//lullaby/util:expected",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:optional",
        "//lullaby/util:span",
        "//lullaby/util:string_view",
        "//lullaby/util:typeid",
        "//lullaby/tools/common:file_utils",
        "//lullaby/tools/common:json_utils",
        "@rapidjson//:rapidjson",
    ],
)

cc_binary(
    name = "shader_pipeline",
    srcs = [
        "main.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":shader_pipeline_lib",
        "//lullaby/util:arg_parser",
        "//lullaby/util:logging",
        "//lullaby/tools/common:file_utils",
    ],
)
