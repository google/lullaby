
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "dispatcher",
    srcs = [
        "dispatcher_system.cc",
        "event.cc",
    ],
    hdrs = [
        "dispatcher_system.h",
        "event.h",
    ],
    deps = [
        "//:fbs",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/script",
        "//lullaby/util:entity",
        "//lullaby/util:logging",
        "//lullaby/util:registry",
        "//lullaby/util:thread_safe_queue",
    ],
)
