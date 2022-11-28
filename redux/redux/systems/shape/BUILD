# ECS System that creates simple geometric shapes for the Render and Physics Systems.

load(
    "//redux/tools:def_cc_library.bzl",
    "def_cc_library",
)

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

def_cc_library(
    name = "shape_def",
    src = "shape_def.def",
)

cc_library(
    name = "shape_builder",
    hdrs = [
        "box_shape_generator.h",
        "shape_builder.h",
        "sphere_shape_generator.h",
    ],
    deps = [
        ":shape_def",
        "//redux/modules/base:data_builder",
        "//redux/modules/base:logging",
        "//redux/modules/graphics:vertex",
    ],
)

cc_test(
    name = "shape_builder_tests",
    srcs = [
        "box_shape_generator_tests.cc",
        "sphere_shape_generator_tests.cc",
    ],
    deps = [
        ":shape_builder",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "shape",
    srcs = [
        "shape_system.cc",
    ],
    hdrs = [
        "shape_system.h",
    ],
    deps = [
        ":shape_builder",
        ":shape_def",
        "@absl//absl/container:flat_hash_map",
        "//redux/modules/base:data_builder",
        "//redux/modules/base:logging",
        "//redux/modules/ecs",
        "//redux/modules/graphics:vertex",
        "//redux/modules/math:bounds",
        "//redux/systems/physics",
        "//redux/systems/render",
    ],
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [":shape"],
    alwayslink = 1,
)
