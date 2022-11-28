# ECS System for storing camera-related settings with an Entity.

load(
    "//redux/tools:def_cc_library.bzl",
    "def_cc_library",
)

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

def_cc_library(
    name = "camera_def",
    src = "camera_def.def",
)

cc_library(
    name = "camera",
    srcs = ["camera_system.cc"],
    hdrs = ["camera_system.h"],
    deps = [
        ":camera_def",
        "@absl//absl/container:flat_hash_map",
        "//redux/engines/platform:device_manager",
        "//redux/engines/render",
        "//redux/modules/base:choreographer",
        "//redux/modules/base:typeid",
        "//redux/modules/ecs",
        "//redux/modules/graphics:camera_ops",
        "//redux/modules/math",
        "//redux/modules/math:bounds",
        "//redux/modules/math:matrix",
        "//redux/modules/math:ray",
        "//redux/systems/transform",
    ],
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [
        ":camera",
        "//redux/modules/ecs",
    ],
    alwayslink = 1,
)
