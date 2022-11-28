# ECS System that stores rig and pose data.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "rig",
    srcs = ["rig_system.cc"],
    hdrs = ["rig_system.h"],
    deps = [
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/types:span",
        "//redux/modules/base:logging",
        "//redux/modules/base:typeid",
        "//redux/modules/ecs",
        "//redux/modules/math:matrix",
    ],
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [
        ":rig",
        "//redux/modules/ecs",
    ],
    alwayslink = 1,
)
