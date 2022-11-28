# ECS System that stores key/values pairs with an Entity.

load(
    "//redux/tools:def_cc_library.bzl",
    "def_cc_library",
)

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

def_cc_library(
    name = "datastore_def",
    src = "datastore_def.def",
)

cc_library(
    name = "datastore",
    srcs = ["datastore_system.cc"],
    hdrs = ["datastore_system.h"],
    deps = [
        ":datastore_def",
        "@absl//absl/container:flat_hash_map",
        "//redux/engines/script:function_binder",
        "//redux/modules/base:typeid",
        "//redux/modules/ecs",
        "//redux/modules/var",
    ],
)

cc_test(
    name = "datastore_system_tests",
    srcs = ["datastore_system_tests.cc"],
    deps = [
        ":datastore",
        "@gtest//:gtest_main",
        "//redux/engines/script/redux",
    ],
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [
        ":datastore",
        "//redux/modules/ecs",
    ],
    alwayslink = 1,
)
