
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "word_art",
    srcs = [
        "word_art_system.cc",
    ],
    hdrs = [
        "word_art_system.h",
    ],
    deps = [
        "@absl//absl/strings",
        "//:fbs",
        "//lullaby/events",
        "//lullaby/modules/animation_channels:render_channels",
        "//lullaby/modules/ecs",
        "//lullaby/modules/file",
        "//lullaby/modules/lullscript",
        "//lullaby/modules/script",
        "//lullaby/systems/animation",
        "//lullaby/systems/audio",
        "//lullaby/systems/model_asset",
        "//lullaby/systems/render",
        "//lullaby/systems/transform",
        "//lullaby/util:entity",
        "//lullaby/util:filename",
        "//lullaby/util:make_unique",
        "//lullaby/util:random_number_generator",
        "//lullaby/util:registry",
        "//lullaby/util:scheduled_processor",
        "@mathfu//:mathfu",
    ],
)
