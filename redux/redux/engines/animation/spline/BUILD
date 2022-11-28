# Spline library for animation engine.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_test(
    name = "bulk_spline_evaluator_tests",
    srcs = ["bulk_spline_evaluator_tests.cc"],
    deps = [
        ":compact_spline",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "compact_spline",
    srcs = [
        "bulk_spline_evaluator.cc",
        "compact_spline.cc",
    ],
    hdrs = [
        "bulk_spline_evaluator.h",
        "compact_spline.h",
        "compact_spline_node.h",
    ],
    deps = [
        ":cubic_curve",
        ":dual_cubic",
        "@absl//absl/base:core_headers",
        "@absl//absl/time",
        "//redux/modules/math",
        "//redux/modules/math:bounds",
        "//redux/modules/math:interpolation",
        "//redux/modules/math:vector",
    ],
)

cc_test(
    name = "compact_spline_tests",
    srcs = ["compact_spline_tests.cc"],
    deps = [
        ":compact_spline",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "cubic_curve",
    srcs = [
        "cubic_curve.cc",
    ],
    hdrs = [
        "cubic_curve.h",
    ],
    deps = [
        "//redux/modules/math:bounds",
        "//redux/modules/math:float",
        "//redux/modules/math:vector",
    ],
)

cc_test(
    name = "cubic_curve_tests",
    srcs = ["cubic_curve_tests.cc"],
    deps = [
        ":cubic_curve",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "dual_cubic",
    srcs = [
        "dual_cubic.cc",
    ],
    hdrs = [
        "dual_cubic.h",
    ],
    deps = [
        ":cubic_curve",
        ":quadratic_curve",
        "//redux/modules/math:bounds",
        "//redux/modules/math:interpolation",
    ],
)

cc_library(
    name = "quadratic_curve",
    srcs = [
        "quadratic_curve.cc",
    ],
    hdrs = [
        "quadratic_curve.h",
    ],
    deps = [
        "//redux/modules/base:logging",
        "//redux/modules/math:bounds",
        "//redux/modules/math:float",
        "//redux/modules/math:vector",
    ],
)

cc_test(
    name = "quadratic_curve_tests",
    srcs = ["quadratic_curve_tests.cc"],
    deps = [
        ":quadratic_curve",
        "@gtest//:gtest_main",
    ],
)
