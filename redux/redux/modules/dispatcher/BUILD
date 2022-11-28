# A messaging/event framework for decoupling communication between objects.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "dispatcher",
    srcs = ["dispatcher.cc"],
    hdrs = ["dispatcher.h"],
    deps = [
        ":message",
        "@absl//absl/container:flat_hash_map",
        "//redux/modules/base:function_traits",
        "//redux/modules/base:typeid",
    ],
)

cc_test(
    name = "dispatcher_tests",
    srcs = ["dispatcher_tests.cc"],
    deps = [
        ":dispatcher",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "message",
    srcs = ["message.cc"],
    hdrs = ["message.h"],
    deps = [
        "@absl//absl/types:any",
        "//redux/modules/base:logging",
        "//redux/modules/base:serialize",
        "//redux/modules/base:typed_ptr",
        "//redux/modules/base:typeid",
        "//redux/modules/var",
        "//redux/modules/var:var_serializer",
    ],
)

cc_test(
    name = "message_tests",
    srcs = ["message_tests.cc"],
    deps = [
        ":message",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "queued_dispatcher",
    srcs = ["queued_dispatcher.cc"],
    hdrs = ["queued_dispatcher.h"],
    deps = [
        ":dispatcher",
        "//redux/modules/base:thread_safe_deque",
    ],
)

cc_test(
    name = "queued_dispatcher_tests",
    srcs = ["queued_dispatcher_tests.cc"],
    deps = [
        ":queued_dispatcher",
        "@gtest//:gtest_main",
    ],
)
