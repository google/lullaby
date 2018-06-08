# BUILD file for a LullScript compiler.

licenses(["notice"])  # Apache 2.0

cc_binary(
    name = "lull_script_compiler",
    srcs = [
        "lull_script_compiler.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//lullaby/modules/lullscript",
    ],
)
