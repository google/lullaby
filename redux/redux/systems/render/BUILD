# ECS System for rendering Entities.

load(
    "//redux/tools:def_cc_library.bzl",
    "def_cc_library",
)

licenses(["notice"])

package(
    
    default_visibility = ["//redux:visibility"],
)

def_cc_library(
    name = "render_def",
    src = "render_def.def",
    deps = [
        "//redux/modules/graphics:enums",
    ],
)

cc_library(
    name = "render",
    srcs = ["render_system.cc"],
    hdrs = ["render_system.h"],
    deps = [
        ":render_def",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/log",
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
        "//redux/modules/graphics:mesh_data",
        "//redux/modules/graphics:texture_usage",
        "//redux/modules/math:matrix",
        "//redux/modules/math:transform",
        "//redux/modules/math:vector",
        "//redux/systems/rig",
        "//redux/systems/transform",
    ],
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [":render"],
    alwayslink = 1,
)
