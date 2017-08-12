
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
