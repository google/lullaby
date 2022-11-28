# Common graphics-related types and functions.

load(
    "//redux/tools:flatbuffer_cc_library.bzl",
    "flatbuffer_cc_library",
)

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "camera_ops",
    srcs = ["camera_ops.cc"],
    hdrs = ["camera_ops.h"],
    deps = [
        "//redux/modules/math:bounds",
        "//redux/modules/math:matrix",
        "//redux/modules/math:quaternion",
        "//redux/modules/math:ray",
        "//redux/modules/math:transform",
        "//redux/modules/math:vector",
    ],
)

cc_test(
    name = "camera_ops_tests",
    srcs = ["camera_ops_tests.cc"],
    deps = [
        ":camera_ops",
        "@gtest//:gtest_main",
        "//redux/modules/math",
    ],
)

cc_library(
    name = "color",
    srcs = ["color.cc"],
    hdrs = ["color.h"],
    deps = [
        "//redux/modules/math:vector",
    ],
)

cc_test(
    name = "color_tests",
    srcs = ["color_tests.cc"],
    deps = [
        ":color",
        "@gtest//:gtest_main",
    ],
)

flatbuffer_cc_library(
    name = "graphics_enums_fbs",
    srcs = ["graphics_enums.fbs"],
)

cc_library(
    name = "enums",
    hdrs = ["enums.h"],
    deps = [
        ":graphics_enums_fbs",
        "//redux/modules/base:hash",
        "//redux/modules/base:logging",
    ],
)

cc_library(
    name = "image_atlaser",
    srcs = ["image_atlaser.cc"],
    hdrs = ["image_atlaser.h"],
    deps = [
        ":enums",
        ":image_data",
        ":image_utils",
        "@absl//absl/container:flat_hash_map",
        "//redux/modules/base:data_container",
        "//redux/modules/base:hash",
        "//redux/modules/base:logging",
        "//redux/modules/math:bounds",
    ],
)

cc_test(
    name = "image_atlaser_tests",
    srcs = ["image_atlaser_tests.cc"],
    deps = [
        ":image_atlaser",
        "@gtest//:gtest_main",
        "//redux/modules/base:data_builder",
        "//redux/modules/math",
    ],
)

cc_library(
    name = "image_data",
    srcs = ["image_data.cc"],
    hdrs = ["image_data.h"],
    deps = [
        ":enums",
        ":image_utils",
        "//redux/modules/base:data_container",
        "//redux/modules/base:logging",
        "//redux/modules/math:vector",
    ],
)

cc_library(
    name = "image_utils",
    srcs = ["image_utils.cc"],
    hdrs = ["image_utils.h"],
    deps = [
        ":enums",
        "@absl//absl/types:span",
        "//redux/modules/math:vector",
    ],
)

cc_library(
    name = "material_data",
    hdrs = ["material_data.h"],
    deps = [
        ":enums",
        ":texture_usage",
        "//redux/modules/var",
    ],
)

cc_library(
    name = "mesh_data",
    srcs = ["mesh_data.cc"],
    hdrs = ["mesh_data.h"],
    deps = [
        ":vertex_attribute",
        ":vertex_format",
        "//redux/modules/base:data_container",
        "//redux/modules/base:hash",
        "//redux/modules/base:logging",
        "//redux/modules/math:bounds",
    ],
)

cc_library(
    name = "texture_usage",
    hdrs = ["texture_usage.h"],
    deps = [
        ":enums",
        "@absl//absl/types:span",
        "//redux/modules/base:hash",
        "//redux/modules/base:logging",
    ],
)

cc_library(
    name = "vertex",
    hdrs = [
        "vertex.h",
        "vertex_layout.h",
        "vertex_utils.h",
    ],
    deps = [
        ":enums",
        ":vertex_format",
        "//redux/modules/math",
        "//redux/modules/math:matrix",
        "//redux/modules/math:quaternion",
        "//redux/modules/math:vector",
    ],
)

cc_test(
    name = "vertex_tests",
    srcs = ["vertex_tests.cc"],
    deps = [
        ":vertex",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "vertex_attribute",
    hdrs = ["vertex_attribute.h"],
    deps = [
        ":enums",
    ],
)

cc_library(
    name = "vertex_format",
    srcs = ["vertex_format.cc"],
    hdrs = ["vertex_format.h"],
    deps = [
        ":vertex_attribute",
        "//redux/modules/base:logging",
    ],
)

cc_test(
    name = "vertex_format_tests",
    srcs = ["vertex_format_tests.cc"],
    deps = [
        ":vertex_format",
        "@gtest//:gtest_main",
    ],
)
