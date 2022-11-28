# Common math types and functions.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "bounds",
    hdrs = ["bounds.h"],
    deps = [
        ":math",
        ":vector",
        "@absl//absl/types:span",
        "//redux/modules/base:logging",
        "//redux/modules/base:typeid",
    ],
)

cc_test(
    name = "bounds_tests",
    srcs = ["bounds_tests.cc"],
    deps = [
        ":bounds",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "constants",
    hdrs = ["constants.h"],
)

cc_library(
    name = "float",
    srcs = ["float.cc"],
    hdrs = ["float.h"],
)

cc_test(
    name = "float_tests",
    srcs = ["float_tests.cc"],
    deps = [
        ":float",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "interpolation",
    srcs = ["interpolation.cc"],
    hdrs = ["interpolation.h"],
    deps = [":math"],
)

cc_test(
    name = "interpolation_tests",
    srcs = ["interpolation_tests.cc"],
    deps = [
        ":interpolation",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "math",
    hdrs = ["math.h"],
    deps = [
        ":constants",
        "//redux/modules/base:logging",
    ],
)

cc_test(
    name = "math_tests",
    srcs = ["math_tests.cc"],
    deps = [
        ":constants",
        ":math",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "matrix",
    hdrs = [
        "detail/matrix_layout.h",
        "matrix.h",
    ],
    deps = [
        ":constants",
        ":vector",
        "//redux/modules/base:logging",
        "//redux/modules/base:typeid",
        "@vectorial//:vectorial",
    ],
)

cc_test(
    name = "matrix_tests",
    srcs = ["matrix_tests.cc"],
    deps = [
        ":constants",
        ":matrix",
        ":vector",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "quaternion",
    hdrs = [
        "detail/quaternion_layout.h",
        "quaternion.h",
    ],
    deps = [
        ":constants",
        ":math",
        ":matrix",
        ":vector",
        "//redux/modules/base:typeid",
        "@vectorial//:vectorial",
    ],
)

cc_test(
    name = "quaternion_tests",
    srcs = ["quaternion_tests.cc"],
    deps = [
        ":constants",
        ":matrix",
        ":quaternion",
        ":vector",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "ray",
    hdrs = ["ray.h"],
    deps = [
        ":matrix",
        ":vector",
        "//redux/modules/base:hash",
        "//redux/modules/base:typeid",
    ],
)

cc_test(
    name = "ray_tests",
    srcs = ["ray_tests.cc"],
    deps = [
        ":quaternion",
        ":ray",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "testing",
    testonly = 1,
    hdrs = [
        "testing.h",
    ],
    deps = [
        ":matrix",
        ":quaternion",
        ":vector",
        "@gtest//:gtest",
    ],
)

cc_library(
    name = "transform",
    hdrs = ["transform.h"],
    deps = [
        ":matrix",
        ":quaternion",
        ":vector",
        "//redux/modules/base:hash",
        "//redux/modules/base:typeid",
    ],
)

cc_test(
    name = "transform_tests",
    srcs = ["transform_tests.cc"],
    deps = [
        ":transform",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "vector",
    hdrs = [
        "detail/vector_layout.h",
        "vector.h",
    ],
    deps = [
        ":constants",
        "//redux/modules/base:typeid",
        "@vectorial//:vectorial",
    ],
)

cc_test(
    name = "vector_tests",
    srcs = ["vector_tests.cc"],
    deps = [
        ":vector",
        "@gtest//:gtest_main",
    ],
)
