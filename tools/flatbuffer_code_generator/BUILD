# BUILD file for lullaby's flatbuffer fbs code generator.

licenses(["notice"])  # Apache 2.0

cc_binary(
    name = "flatbuffer_code_generator",
    srcs = [
        "flatc.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@flatbuffers//:flatc_library",
    ],
)
