
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "reticle",
    srcs = [
        "reticle_util.cc",
    ],
    hdrs = [
        "reticle_provider.h",
        "reticle_util.h",
    ],
    deps = [
        "//lullaby/modules/ecs:entity",
        "//lullaby/modules/input",
        "//lullaby/systems/transform",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "@mathfu//:mathfu",
    ],
)
