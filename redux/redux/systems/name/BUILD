# ECS System for associating string names with Entities.

load(
    "//redux/tools:def_cc_library.bzl",
    "def_cc_library",
)

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

def_cc_library(
    name = "name_def",
    src = "name_def.def",
)

cc_library(
    name = "name",
    srcs = ["name_system.cc"],
    hdrs = ["name_system.h"],
    deps = [
        ":name_def",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/functional:function_ref",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//redux/engines/script:function_binder",
        "//redux/modules/base:typeid",
        "//redux/modules/ecs",
    ],
)

cc_test(
    name = "name_system_tests",
    srcs = ["name_system_tests.cc"],
    deps = [
        ":name",
        "@gtest//:gtest_main",
        "//redux/engines/script",
        "//redux/engines/script/redux",
    ],
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [
        ":name",
        "//redux/modules/ecs",
    ],
    alwayslink = 1,
)
