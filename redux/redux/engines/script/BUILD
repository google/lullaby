# Scripting engine.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "script",
    hdrs = [
        "script.h",
        "script_call_context.h",
        "script_engine.h",
    ],
    deps = [
        ":call_native_function",
        "@absl//absl/status",
        "//redux/modules/base:hash",
        "//redux/modules/base:registry",
        "//redux/modules/base:typeid",
        "//redux/modules/var",
        "//redux/modules/var:var_convert",
        "@magic_enum//:magic_enum",
    ],
)

cc_library(
    name = "call_native_function",
    hdrs = ["call_native_function.h"],
    deps = [
        "@absl//absl/status",
        "//redux/modules/base:function_traits",
        "//redux/modules/base:logging",
    ],
)

cc_test(
    name = "call_native_function_tests",
    srcs = ["call_native_function_tests.cc"],
    deps = [
        ":call_native_function",
        "@gtest//:gtest_main",
        "@absl//absl/types:any",
    ],
)

cc_library(
    name = "function_binder",
    srcs = ["function_binder.cc"],
    hdrs = ["function_binder.h"],
    deps = [":script"],
)
