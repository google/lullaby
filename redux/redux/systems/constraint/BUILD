# ECS System for attaching Entities to other Entities.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "events",
    hdrs = ["events.h"],
    deps = [
        "//redux/modules/base:hash",
        "//redux/modules/base:typeid",
        "//redux/modules/ecs:entity",
    ],
)

cc_library(
    name = "constraint",
    srcs = ["constraint_system.cc"],
    hdrs = ["constraint_system.h"],
    deps = [
        ":events",
        "@absl//absl/types:span",
        "//redux/engines/script:function_binder",
        "//redux/modules/base:choreographer",
        "//redux/modules/base:data_table",
        "//redux/modules/base:typeid",
        "//redux/modules/ecs",
        "//redux/modules/math:matrix",
        "//redux/modules/math:transform",
        "//redux/modules/math:vector",
        "//redux/systems/dispatcher",
        "//redux/systems/rig",
        "//redux/systems/transform",
    ],
)

cc_test(
    name = "constraint_system_tests",
    srcs = ["constraint_system_tests.cc"],
    deps = [
        ":constraint",
        "@gtest//:gtest_main",
        "//redux/engines/script/redux",
        "//redux/modules/math:testing",
    ],
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [
        ":constraint",
        "//redux/modules/ecs",
    ],
    alwayslink = 1,
)
