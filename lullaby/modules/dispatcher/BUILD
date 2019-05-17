
package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "dispatcher",
    srcs = [
        "dispatcher.cc",
        "event_wrapper.cc",
        "queued_dispatcher.cc",
    ],
    hdrs = [
        "dispatcher.h",
        "event_wrapper.h",
        "queued_dispatcher.h",
    ],
    deps = [
        "//lullaby/modules/serialize",
        "//lullaby/util:aligned_alloc",
        "//lullaby/util:macros",
        "//lullaby/util:string_view",
        "//lullaby/util:thread_safe_queue",
        "//lullaby/util:typeid",
        "//lullaby/util:variant",
    ],
)

cc_library(
    name = "dispatcher_binder",
    srcs = [
        "dispatcher_binder.cc",
    ],
    hdrs = [
        "dispatcher_binder.h",
    ],
    deps = [
        ":dispatcher",
        "//lullaby/modules/script",
        "//lullaby/util:logging",
        "//lullaby/util:registry",
        "//lullaby/util:typeid",
    ],
)

cc_library(
    name = "dispatcher_jni",
    srcs = select({
        "//:target_os_android": [
            "dispatcher_jni.cc",
        ],
        "//conditions:default": [],
    }),
    hdrs = [
        "dispatcher_jni.h",
    ],
    deps = [
        ":dispatcher",
        ":dispatcher_binder",
        "//lullaby/modules/jni:jni_context",
        "//lullaby/modules/jni:jni_convert",
        "//lullaby/modules/jni:jni_util",
        "//lullaby/modules/jni:registry_jni",
        "//lullaby/modules/jni:scoped_java_global_ref",
        "//lullaby/util:registry",
    ],
)
