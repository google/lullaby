# Test-only utilities.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "testing",
    testonly = True,
    srcs = ["testing.cc"],
    hdrs = ["testing.h"],
    deps = [
        "@gtest//:gtest",
        "//redux/modules/base:filepath",
        "//redux/modules/base:logging",
    ],
)
