# BUILD file for test data.

licenses(["notice"])  # Apache 2.0

load(
    "//third_party/lullaby:build_defs.bzl",
    "lullaby_generate_schema_cc_library",
)
load(
    "@flatbuffers//:flatbuffers:build_defs.bzl",
    "flatbuffer_cc_library",
)

filegroup(
    name = "testdata",
    srcs = glob([
        "*.json",
    ]),
    visibility = ["//visibility:public"],
)

filegroup(
    name = "flatc_schema",
    srcs = [
        "flatc.fbs",
        "flatc_ref.fbs",
    ],
    visibility = ["//visibility:public"],
)

lullaby_generate_schema_cc_library(
    name = "flatc_fbs",
    srcs = [
        "flatc.fbs",
    ],
    visibility = ["//visibility:public"],
)

# A version of the above fbs file, but one that just uses the default flatc
# library.  This is used to perform benchmarks comparing the lull flatc
# code generator and serialization against the default flatbuffer
# implementations.
flatbuffer_cc_library(
    name = "flatc_ref_fbs",
    srcs = [
        "flatc_ref.fbs",
    ],
    flatc_args = [
        "--no-union-value-namespacing",
        "--gen-mutable",
        "--gen-name-strings",
        "--gen-object-api",
    ],
    visibility = ["//visibility:public"],
)
