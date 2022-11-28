# ECS System for programatically animating values over time.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "tween",
    srcs = ["tween_system.cc"],
    hdrs = ["tween_system.h"],
    deps = [
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/time",
        "//redux/engines/animation",
        "//redux/engines/animation/spline:compact_spline",
        "//redux/engines/script:function_binder",
        "//redux/modules/base:choreographer",
        "//redux/modules/base:typeid",
        "//redux/modules/ecs",
        "//redux/modules/math:interpolation",
    ],
)

cc_test(
    name = "tween_system_tests",
    srcs = ["tween_system_tests.cc"],
    deps = [
        ":tween",
        "@gtest//:gtest_main",
        "//redux/engines/script/redux",
    ],
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [
        ":tween",
        "//redux/modules/ecs",
    ],
    alwayslink = 1,
)
