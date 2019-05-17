
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
        "//lullaby/modules/dispatcher:dispatcher_binder",
        "//lullaby/modules/ecs",
        "//lullaby/modules/flatbuffers",
        "//lullaby/modules/script",
        "//lullaby/util:entity",
        "//lullaby/util:logging",
        "//lullaby/util:registry",
        "//lullaby/util:thread_safe_queue",
    ],
)

cc_library(
    name = "dispatcher_jni",
    srcs = select({
        "//:target_os_android": [
            "dispatcher_system_jni.cc",
        ],
        "//conditions:default": [],
    }),
    deps = [
        ":dispatcher",
        "//lullaby/modules/dispatcher:dispatcher_jni",
        "//lullaby/modules/ecs",
        "//lullaby/modules/jni:jni_util",
        "//lullaby/modules/jni:registry_jni",
    ],
)
