cc_library(
    name = "hello_world_lib",
    srcs = [
        "src/hello_world.cc",
    ],
    hdrs = [
        "src/hello_world.h",
    ],
    deps = [
        ":generated",
        "//examples/example_app",
        "//lullaby/systems/render:next",
        "//lullaby/systems/text:flatui",
        "//lullaby/systems/transform",
    ],
    alwayslink = 1,
)
cc_binary(
    name = "hello_world_desktop",
    deps = [
        ":hello_world_lib",
        "//examples/example_app:sdl2_main",
    ],
    linkopts = [
        "-lGL",
        "-ldl",
        "-lrt",
        "-lm",
        "-lpthread",
    ],
)
cc_library(
    name = "generated",
    hdrs = [
        "entity_generated.h",
    ],
    deps = [
        "@flatbuffers//:flatbuffers",
        "//lullaby/generated"
    ]
)
