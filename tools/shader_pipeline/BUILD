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
        "//third_party/jsonnet:libjsonnet",
        "//lullaby/util:logging",
        "//lullaby/util:optional",
        "//lullaby/util:span",
        "//lullaby/util:string_view",
        "//third_party/lullaby/tools/common",
        "//third_party/lullaby/tools/common:json_utils",
        "//third_party/rapidjson",
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
        "//third_party/lullaby/tools/common",
    ],
)
