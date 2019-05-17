# BUILD file for Lullaby's BlueprintDef flatbuffer binary generator.

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "compile_blueprint_from_json_lib",
    srcs = [
        "blueprint_from_json_compiler.cc",
    ],
    hdrs = [
        "blueprint_from_json_compiler.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@flatbuffers//:flatbuffers",
        "//lullaby/modules/ecs",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:make_unique",
        "//lullaby/util:optional",
        "//lullaby/util:span",
        "//lullaby/util:string_view",
        "@rapidjson//:rapidjson",
    ],
)

cc_binary(
    name = "compile_blueprint_from_json",
    srcs = [
        "main.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":compile_blueprint_from_json_lib",
        "//lullaby/util:arg_parser",
        "//lullaby/util:filename",
        "//lullaby/util:logging",
        "//lullaby/tools/common:file_utils",
    ],
)
