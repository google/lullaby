# ECS System for managing lights.

load(
    "//redux/tools:def_cc_library.bzl",
    "def_cc_library",
)

licenses(["notice"])

package(
    
    default_visibility = ["//redux:visibility"],
)

def_cc_library(
    name = "light_def",
    src = "light_def.def",
)

cc_library(
    name = "light",
    srcs = ["light_system.cc"],
    hdrs = ["light_system.h"],
    deps = [
        ":light_def",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/log:check",
        "@absl//absl/types:span",
        "//redux/engines/render",
        "//redux/modules/base:choreographer",
        "//redux/modules/base:hash",
        "//redux/modules/base:registry",
        "//redux/modules/base:typeid",
        "//redux/modules/ecs",
        "//redux/modules/ecs:entity",
        "//redux/modules/graphics:color",
        "//redux/modules/graphics:graphics_enums_fbs",
        "//redux/modules/math",
        "//redux/systems/transform",
    ],
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [":light"],
    alwayslink = 1,
)
