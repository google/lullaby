# Common libraries for redux tools.

load(
    "//redux/tools:flatbuffer_cc_library.bzl",
    "flatbuffer_cc_library",
)

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

flatbuffer_cc_library(
    name = "test_fbs",
    srcs = [
        "test_data/schema.fbs",
        "test_data/schema_include.fbs",
    ],
    out_prefix = "test_data/",
)

flatbuffer_cc_library(
    name = "axis_system_fbs",
    srcs = ["axis_system.fbs"],
)

cc_library(
    name = "axis_system",
    hdrs = ["axis_system.h"],
    deps = [":axis_system_fbs"],
)

cc_library(
    name = "assimp_utils",
    srcs = ["assimp_utils.cc"],
    hdrs = ["assimp_utils.h"],
    deps = [
        ":axis_system",
        "@absl//absl/container:flat_hash_set",
        "@assimp//:assimp",
        "//redux/modules/base:logging",
    ],
)

cc_library(
    name = "fbx_utils",
    srcs = ["fbx_utils.cc"],
    hdrs = ["fbx_utils.h"],
    deps = [
        ":axis_system",
        "@absl//absl/container:flat_hash_set",
        "@libfbx//:libfbx",
        "//redux/modules/math:matrix",
        "//redux/modules/math:quaternion",
        "//redux/modules/math:vector",
    ],
)

cc_library(
    name = "file_utils",
    srcs = ["file_utils.cc"],
    hdrs = ["file_utils.h"],
    deps = [
        "@absl//absl/status:statusor",
        "//redux/modules/base:data_builder",
        "//redux/modules/base:data_container",
        "//redux/modules/base:logging",
    ],
)

cc_test(
    name = "file_utils_tests",
    srcs = ["file_utils_tests.cc"],
    data = ["test_data/hello.txt"],
    deps = [
        ":file_utils",
        "@gtest//:gtest_main",
        "//redux/modules/testing",
    ],
)

cc_library(
    name = "flatbuffer_utils",
    srcs = ["flatbuffer_utils.cc"],
    hdrs = ["flatbuffer_utils.h"],
    deps = [
        ":file_utils",
        "@flatbuffers//:flatc_library",
        "//redux/modules/base:data_container",
    ],
)

cc_test(
    name = "flatbuffer_utils_tests",
    srcs = ["flatbuffer_utils_tests.cc"],
    data = [
        "test_data/schema.fbs",
        "test_data/schema_include.fbs",
    ],
    deps = [
        ":file_utils",
        ":flatbuffer_utils",
        ":test_fbs",
        "@gtest//:gtest_main",
        "//redux/modules/testing",
    ],
)

cc_library(
    name = "json_utils",
    hdrs = ["json_utils.h"],
    deps = [
        "//redux/modules/base:hash",
        "//redux/modules/base:logging",
        "//redux/modules/base:serialize_traits",
        "@magic_enum//:magic_enum",
        "@rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "json_utils_tests",
    srcs = ["json_utils_tests.cc"],
    deps = [
        ":file_utils",
        ":json_utils",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "jsonnet_utils",
    srcs = ["jsonnet_utils.cc"],
    hdrs = ["jsonnet_utils.h"],
    deps = [
        ":file_utils",
        "@absl//absl/container:flat_hash_map",
        "@jsonnet//core:libjsonnet",
        "//redux/modules/base:logging",
    ],
)

cc_test(
    name = "jsonnet_utils_tests",
    srcs = ["jsonnet_utils_tests.cc"],
    data = ["test_data/sum.jsonnet"],
    deps = [
        ":file_utils",
        ":jsonnet_utils",
        "@gtest//:gtest_main",
        "//redux/modules/testing",
    ],
)

cc_library(
    name = "log_utils",
    srcs = ["log_utils.cc"],
    hdrs = ["log_utils.h"],
    deps = [
        "//redux/modules/base:hash",
        "//redux/modules/base:logging",
        "//redux/modules/flatbuffers:common",
        "//redux/modules/flatbuffers:math",
        "//redux/modules/flatbuffers:var",
        "//redux/modules/graphics:enums",
        "//redux/modules/math:matrix",
        "//redux/modules/math:quaternion",
        "//redux/modules/math:vector",
    ],
)
