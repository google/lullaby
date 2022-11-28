# ECS System for attaching scripts to Entities.

load(
    "//redux/tools:def_cc_library.bzl",
    "def_cc_library",
)

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

def_cc_library(
    name = "script_def",
    src = "script_def.def",
)

cc_library(
    name = "script",
    srcs = ["script_system.cc"],
    hdrs = ["script_system.h"],
    deps = [
        ":script_def",
        "//redux/engines/script",
        "//redux/modules/base:choreographer",
        "//redux/modules/base:typeid",
        "//redux/modules/ecs",
        "//redux/systems/dispatcher",
    ],
)

cc_test(
    name = "script_system_tests",
    srcs = ["script_system_tests.cc"],
    deps = [
        ":script",
        "@gtest//:gtest_main",
        "@absl//absl/functional:bind_front",
        "//redux/engines/script/redux",
        "//redux/systems/dispatcher",
    ],
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [
        ":script",
        "//redux/modules/ecs",
    ],
    alwayslink = 1,
)
