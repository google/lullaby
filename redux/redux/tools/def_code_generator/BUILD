# Generates a C++ header file from a .def file.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "def_code_generator_lib",
    srcs = [
        "code_builder.cc",
        "generate_code.cc",
        "parse_def_file.cc",
    ],
    hdrs = [
        "code_builder.h",
        "def_document.h",
        "generate_code.h",
        "metadata_types.h",
        "parse_def_file.h",
    ],
    deps = [
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/flags:flag",
        "@absl//absl/flags:parse",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "@fmtlib//:fmtlib",
        "//redux/modules/base:filepath",
        "//redux/modules/base:logging",
        "//redux/modules/datafile:datafile_parser",
    ],
)

cc_test(
    name = "parse_def_file_tests",
    srcs = ["parse_def_file_tests.cc"],
    deps = [
        ":def_code_generator_lib",
        "@gtest//:gtest_main",
    ],
)

cc_binary(
    name = "def_code_generator",
    srcs = ["main.cc"],
    deps = [
        ":def_code_generator_lib",
        "@absl//absl/flags:flag",
        "@absl//absl/flags:parse",
        "//redux/modules/base:filepath",
        "//redux/modules/base:logging",
        "//redux/tools/common:file_utils",
    ],
)
