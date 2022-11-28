# A flexible dynamically-typed variant type library.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "var",
    srcs = [
        "var.cc",
        "var_array.cc",
        "var_table.cc",
    ],
    hdrs = [
        "var.h",
        "var_array.h",
        "var_table.h",
    ],
    deps = [
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/time",
        "//redux/modules/base:data_buffer",
        "//redux/modules/base:logging",
        "//redux/modules/base:typeid",
    ],
)

cc_library(
    name = "var_convert",
    hdrs = [
        "var_convert.h",
    ],
    deps = [
        ":var",
        "//redux/modules/base:serialize_traits",
        "//redux/modules/base:typed_ptr",
        "@magic_enum//:magic_enum",
    ],
)

cc_library(
    name = "var_serializer",
    hdrs = [
        "var_serializer.h",
    ],
    deps = [
        ":var",
        ":var_convert",
    ],
)

cc_test(
    name = "var_tests",
    srcs = ["var_tests.cc"],
    deps = [
        ":var",
        "@gtest//:gtest_main",
    ],
)

cc_test(
    name = "var_array_tests",
    srcs = ["var_array_tests.cc"],
    deps = [
        ":var",
        "@gtest//:gtest_main",
    ],
)

cc_test(
    name = "var_convert_tests",
    srcs = ["var_convert_tests.cc"],
    deps = [
        ":var_convert",
        "@gtest//:gtest_main",
    ],
)

cc_test(
    name = "var_serializer_tests",
    srcs = ["var_serializer_tests.cc"],
    deps = [
        ":var_serializer",
        "@gtest//:gtest_main",
        "//redux/modules/base:serialize",
    ],
)

cc_test(
    name = "var_table_tests",
    srcs = ["var_table_tests.cc"],
    deps = [
        ":var",
        "@gtest//:gtest_main",
    ],
)
