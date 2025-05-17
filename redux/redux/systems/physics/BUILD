# ECS System for creating rigid bodies and trigger volumes.

load(
    "//redux/tools:def_cc_library.bzl",
    "def_cc_library",
)

licenses(["notice"])

package(
    
    default_visibility = ["//redux:visibility"],
)

def_cc_library(
    name = "rigid_body_def",
    src = "rigid_body_def.def",
    deps = ["//redux/engines/physics:enums"],
)

def_cc_library(
    name = "trigger_def",
    src = "trigger_def.def",
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
    name = "physics",
    srcs = ["physics_system.cc"],
    hdrs = ["physics_system.h"],
    deps = [
        ":events",
        ":rigid_body_def",
        ":trigger_def",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/log",
        "@absl//absl/log:check",
        "@absl//absl/time",
        "//redux/engines/physics",
        "//redux/engines/physics:physics_enums_fbs",
        "//redux/modules/base:bits",
        "//redux/modules/base:choreographer",
        "//redux/modules/base:hash",
        "//redux/modules/base:registry",
        "//redux/modules/base:typeid",
        "//redux/modules/ecs",
        "//redux/modules/ecs:entity",
        "//redux/modules/math",
        "//redux/modules/math:quaternion",
        "//redux/modules/math:transform",
        "//redux/modules/math:vector",
        "//redux/systems/dispatcher",
        "//redux/systems/transform",
    ],
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [
        ":physics",
        "//redux/modules/ecs",
    ],
    alwayslink = 1,
)
