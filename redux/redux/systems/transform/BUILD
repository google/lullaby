# ECS System that associates transforms and box volumes for spatial queries.

load(
    "//redux/tools:def_cc_library.bzl",
    "def_cc_library",
)

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

def_cc_library(
    name = "transform_def",
    src = "transform_def.def",
)

cc_library(
    name = "transform",
    srcs = ["transform_system.cc"],
    hdrs = ["transform_system.h"],
    deps = [
        ":transform_def",
        "//redux/engines/script:function_binder",
        "//redux/modules/base:bits",
        "//redux/modules/base:data_table",
        "//redux/modules/base:typeid",
        "//redux/modules/ecs",
        "//redux/modules/math:bounds",
        "//redux/modules/math:matrix",
        "//redux/modules/math:quaternion",
        "//redux/modules/math:transform",
        "//redux/modules/math:vector",
    ],
)

cc_test(
    name = "transform_system_tests",
    srcs = ["transform_system_tests.cc"],
    deps = [
        ":transform",
        "@gtest//:gtest_main",
        "//redux/engines/script/redux",
    ],
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [
        ":transform",
        "//redux/modules/ecs",
    ],
    alwayslink = 1,
)
