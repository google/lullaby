# A YAML/JSON-like file format with redux-specific functionality.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "datafile_reader",
    srcs = ["datafile_reader.cc"],
    hdrs = ["datafile_reader.h"],
    deps = [
        ":datafile_parser",
        "//redux/engines/script/redux:script_env",
        "//redux/modules/base:logging",
        "//redux/modules/base:serialize",
        "//redux/modules/base:serialize_traits",
        "//redux/modules/var",
    ],
)

cc_test(
    name = "datafile_reader_tests",
    srcs = ["datafile_reader_tests.cc"],
    deps = [
        ":datafile_reader",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "datafile_parser",
    srcs = ["datafile_parser.cc"],
    hdrs = ["datafile_parser.h"],
    deps = ["//redux/modules/base:logging"],
)

cc_test(
    name = "datafile_parser_tests",
    srcs = ["datafile_parser_tests.cc"],
    deps = [
        ":datafile_parser",
        "@gtest//:gtest_main",
        "//redux/modules/var",
    ],
)
