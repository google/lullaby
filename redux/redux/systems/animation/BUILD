# ECS System for playing animation files to update Entity poses.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "animation",
    srcs = ["animation_system.cc"],
    hdrs = ["animation_system.h"],
    deps = [
        "@absl//absl/container:flat_hash_map",
        "//redux/engines/animation",
        "//redux/engines/animation/spline:compact_spline",
        "//redux/modules/base:choreographer",
        "//redux/modules/base:typeid",
        "//redux/modules/ecs",
        "//redux/modules/math:interpolation",
        "//redux/systems/rig",
    ],
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [
        ":animation",
        "//redux/modules/ecs",
    ],
    alwayslink = 1,
)
