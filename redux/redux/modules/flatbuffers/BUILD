# Redux-specific types and extensions for flatbuffers.

load(
    "//redux/tools:flatbuffer_cc_library.bzl",
    "flatbuffer_cc_library",
)

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

flatbuffer_cc_library(
    name = "common_fbs",
    srcs = ["common.fbs"],
)

cc_library(
    name = "common",
    hdrs = ["common.h"],
    deps = [
        ":common_fbs",
        "//redux/modules/base:hash",
    ],
)

cc_test(
    name = "common_tests",
    srcs = ["common_tests.cc"],
    deps = [
        ":common",
        "@gtest//:gtest_main",
    ],
)

flatbuffer_cc_library(
    name = "math_fbs",
    srcs = ["math.fbs"],
)

cc_library(
    name = "math",
    hdrs = ["math.h"],
    deps = [
        ":common",
        ":math_fbs",
        "//redux/modules/math:bounds",
        "//redux/modules/math:matrix",
        "//redux/modules/math:quaternion",
        "//redux/modules/math:vector",
    ],
)

cc_test(
    name = "math_tests",
    srcs = ["math_tests.cc"],
    deps = [
        ":math",
        "@gtest//:gtest_main",
    ],
)

flatbuffer_cc_library(
    name = "var_fbs",
    srcs = ["var.fbs"],
    deps = [
        ":common_fbs",
        ":math_fbs",
    ],
)

cc_library(
    name = "var",
    srcs = ["var.cc"],
    hdrs = ["var.h"],
    deps = [
        ":common",
        ":math",
        ":var_fbs",
        "//redux/modules/base:data_buffer",
        "//redux/modules/base:logging",
        "//redux/modules/math:vector",
        "//redux/modules/var",
    ],
)

cc_test(
    name = "var_tests",
    srcs = ["var_tests.cc"],
    deps = [
        ":var",
        "@gtest//:gtest_main",
        "//redux/modules/math:quaternion",
        "//redux/modules/math:vector",
    ],
)
