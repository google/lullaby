# Physics engine.

load(
    "//redux/tools:flatbuffer_cc_library.bzl",
    "flatbuffer_cc_library",
)

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "physics",
    srcs = [
        "collision_data.cc",
    ],
    hdrs = [
        "collision_data.h",
        "collision_shape.h",
        "physics_engine.h",
        "rigid_body.h",
        "trigger_volume.h",
    ],
    deps = [
        ":enums",
        "@absl//absl/time",
        "//redux/modules/base:bits",
        "//redux/modules/base:data_container",
        "//redux/modules/base:registry",
        "//redux/modules/base:resource_manager",
        "//redux/modules/ecs:entity",
        "//redux/modules/math:bounds",
        "//redux/modules/math:quaternion",
        "//redux/modules/math:transform",
        "//redux/modules/math:vector",
    ],
)

flatbuffer_cc_library(
    name = "physics_enums_fbs",
    srcs = ["physics_enums.fbs"],
)

cc_library(
    name = "enums",
    hdrs = ["enums.h"],
    deps = [
        ":physics_enums_fbs",
        "//redux/modules/base:typeid",
    ],
)
