# ECS System for playing sounds associated with Entities.

load(
    "//redux/tools:def_cc_library.bzl",
    "def_cc_library",
)

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

def_cc_library(
    name = "sound_def",
    src = "sound_def.def",
    deps = ["//redux/modules/audio:enums"],
)

cc_library(
    name = "audio",
    srcs = ["audio_system.cc"],
    hdrs = ["audio_system.h"],
    deps = [
        ":sound_def",
        "@absl//absl/container:flat_hash_map",
        "//redux/engines/audio",
        "//redux/modules/base:choreographer",
        "//redux/modules/base:hash",
        "//redux/modules/base:typeid",
        "//redux/modules/ecs",
        "//redux/modules/math",
        "//redux/systems/transform",
    ],
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [
        ":audio",
        "//redux/modules/ecs",
    ],
    alwayslink = 1,
)
