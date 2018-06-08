
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0


cc_library(
    name = "audio",
    srcs = [
        "audio_system.cc",
        "sound_asset.cc",
        "sound_asset_manager.cc",
    ],
    hdrs = [
        "audio_system.h",
        "play_sound_parameters.h",
        "sound_asset.h",
        "sound_asset_manager.h",
    ],
    deps = [
        "//lullaby/events",
        "//lullaby/util:entity",
        "//lullaby/util:async_processor",
        "//lullaby/util:registry",
        "//lullaby/util:resource_manager",
        "//lullaby/util:thread_safe_queue",
        "//lullaby/util:unordered_vector_map",
        "@mathfu//:mathfu",
        "@gvr//:gvr_audio",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/file",
        "//lullaby/modules/file:file_loader",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/gvr",
        "//lullaby/modules/script",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/transform",
        "//lullaby/util:android_context",
        "//lullaby/util:filename",
        "//lullaby/util:hash",
        "//lullaby/util:logging",
        "//lullaby/util:make_unique",
        "//lullaby/util:math",
        "//lullaby/util:random_number_generator",
        "//lullaby/util:string_view",
        "//lullaby/util:trace",
        "//lullaby/util:typeid",
        "//:fbs",
    ],
)
