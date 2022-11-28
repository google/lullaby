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
)

cc_library(
    name = "render",
    srcs = ["render_system.cc"],
    hdrs = ["render_system.h"],
    deps = [
        ":render_def",
        "//redux/engines/render",
        "//redux/modules/base:choreographer",
        "//redux/modules/base:hash",
        "//redux/modules/base:typeid",
        "//redux/modules/ecs",
        "//redux/modules/math",
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
