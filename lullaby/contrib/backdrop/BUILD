
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "backdrop",
    srcs = [
        "backdrop_channels.cc",
        "backdrop_system.cc",
    ],
    hdrs = [
        "backdrop_channels.h",
        "backdrop_system.h",
    ],
    deps = [
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/render:mesh_util",
        "//lullaby/modules/script",
        "//lullaby/systems/animation",
        "//lullaby/contrib/layout:layout_box",
        "//lullaby/systems/name",
        "//lullaby/systems/nine_patch",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:logging",
        "//lullaby/util:optional",
        "//lullaby/util:registry",
        "@mathfu//:mathfu",
    ],
)
