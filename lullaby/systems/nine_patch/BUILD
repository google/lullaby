
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

public_headers = [
    "nine_patch_system.h",
]

cc_library(
    name = "nine_patch",
    srcs = [
        "nine_patch_system.cc",
    ],
    hdrs = public_headers,
    deps = [
        "//lullaby/generated",
        "//lullaby/events",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/render",
        "//lullaby/systems/layout:layout_box",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:logging",
        "//lullaby/util:optional",
        "@mathfu//:mathfu",
    ],
)
