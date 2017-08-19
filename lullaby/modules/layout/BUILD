
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "layout",
    srcs = [
        "layout.cc",
    ],
    hdrs = [
        "layout.h",
    ],
    deps = [
        "@flatui//:flatui",
        "//lullaby/generated",
        "//lullaby/modules/ecs:entity",
        "//lullaby/systems/layout:layout_box",
        "//lullaby/systems/transform",
        "//lullaby/util:logging",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "@mathfu//:mathfu",
    ],
)
