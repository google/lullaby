# ECS System providing message/event dispatching at both System and Entity levels.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "dispatcher",
    srcs = ["dispatcher_system.cc"],
    hdrs = ["dispatcher_system.h"],
    deps = [
        "//redux/engines/script:function_binder",
        "//redux/modules/base:choreographer",
        "//redux/modules/dispatcher",
        "//redux/modules/dispatcher:queued_dispatcher",
        "//redux/modules/ecs",
    ],
)

cc_test(
    name = "dispatcher_system_tests",
    srcs = ["dispatcher_system_tests.cc"],
    deps = [
        ":dispatcher",
        "@gtest//:gtest_main",
        "//redux/engines/script/redux",
    ],
)

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [
        ":dispatcher",
        "//redux/modules/ecs",
    ],
    alwayslink = 1,
)
