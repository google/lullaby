# ECS System for generating text data for rendering.

load(
    "//redux/tools:def_cc_library.bzl",
    "def_cc_library",
)

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

def_cc_library(
    name = "text_def",
    src = "text_def.def",
    deps = [
        "//redux/engines/text",
        "//redux/modules/math:bounds",
    ],
)

cc_library(
    name = "text",
    srcs = [
        "text_system.cc",
    ],
    hdrs = [
        "text_system.h",
    ],
    deps = [
        ":text_def",
        "@absl//absl/container:flat_hash_map",
        "//redux/engines/render",
        "//redux/engines/text",
        "//redux/modules/base:choreographer",
        "//redux/modules/base:logging",
        "//redux/modules/base:typeid",
        "//redux/modules/ecs",
        "//redux/modules/math:bounds",
        "//redux/systems/render",
    ],
)

# cc_test(
#     name = "text_system_tests",
#     srcs = ["text_system_tests.cc"],
#     deps = [
#         ":text",
#         "@gtest//:gtest_main",
#     ],
# )

cc_library(
    name = "static",
    srcs = ["static_register.cc"],
    deps = [":text"],
    alwayslink = 1,
)
