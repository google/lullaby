# Redux's internal Lisp-based scripting backend for ScriptEngine.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "redux",
    srcs = ["script_engine.cc"],
    deps = [
        ":script_env",
        "@absl//absl/status",
        "//redux/engines/script",
        "//redux/modules/base:asset_loader",
        "//redux/modules/base:registry",
        "//redux/modules/base:static_registry",
        "//redux/modules/ecs:entity",
        "//redux/modules/math:matrix",
        "//redux/modules/math:quaternion",
        "//redux/modules/math:vector",
        "//redux/modules/var",
    ],
)

cc_test(
    name = "script_tests",
    srcs = ["script_tests.cc"],
    deps = [
        "@gtest//:gtest_main",
        "//redux/engines/script",
        "//redux/engines/script/redux",
    ],
)

cc_test(
    name = "script_engine_tests",
    srcs = ["script_engine_tests.cc"],
    deps = [
        "@gtest//:gtest_main",
        "//redux/engines/script",
        "//redux/engines/script/redux",
    ],
)

cc_library(
    name = "script_env",
    srcs = [
        "script_ast_builder.cc",
        "script_compiler.cc",
        "script_env.cc",
        "script_frame.cc",
        "script_parser.cc",
        "script_stack.cc",
        "script_value.cc",
        "stringify.cc",
    ],
    hdrs = [
        "functions/array.h",
        "functions/cond.h",
        "functions/hash.h",
        "functions/map.h",
        "functions/math.h",
        "functions/message.h",
        "functions/operators.h",
        "functions/typeof.h",
        "script_ast_builder.h",
        "script_compiler.h",
        "script_env.h",
        "script_frame.h",
        "script_frame_context.h",
        "script_parser.h",
        "script_stack.h",
        "script_types.h",
        "script_value.h",
    ],
    deps = [
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/status",
        "@absl//absl/strings",
        "@absl//absl/types:span",
        "//redux/engines/script:call_native_function",
        "//redux/modules/base:function_traits",
        "//redux/modules/base:hash",
        "//redux/modules/base:logging",
        "//redux/modules/base:typed_ptr",
        "//redux/modules/base:typeid",
        "//redux/modules/dispatcher:message",
        "//redux/modules/ecs:entity",
        "//redux/modules/math:matrix",
        "//redux/modules/math:quaternion",
        "//redux/modules/math:vector",
        "//redux/modules/var",
        "//redux/modules/var:var_convert",
    ],
)

cc_library(
    name = "testing",
    hdrs = ["testing.h"],
)

cc_test(
    name = "script_compiler_tests",
    srcs = ["script_compiler_tests.cc"],
    deps = [
        ":script_env",
        ":testing",
        "@gtest//:gtest_main",
    ],
)

cc_test(
    name = "script_env_tests",
    srcs = ["script_env_tests.cc"],
    deps = [
        ":script_env",
        ":testing",
        "@gtest//:gtest_main",
    ],
)

cc_test(
    name = "script_parser_tests",
    srcs = ["script_parser_tests.cc"],
    deps = [
        ":script_env",
        ":testing",
        "@gtest//:gtest_main",
        "//redux/modules/var",
    ],
)

cc_test(
    name = "script_stack_tests",
    srcs = ["script_stack_tests.cc"],
    deps = [
        ":script_env",
        ":testing",
        "@gtest//:gtest_main",
    ],
)

cc_test(
    name = "array_tests",
    srcs = ["functions/array_tests.cc"],
    deps = [
        ":script_env",
        ":testing",
        "@gtest//:gtest_main",
    ],
)

cc_test(
    name = "cond_tests",
    srcs = ["functions/cond_tests.cc"],
    deps = [
        ":script_env",
        ":testing",
        "@gtest//:gtest_main",
    ],
)

cc_test(
    name = "map_tests",
    srcs = ["functions/map_tests.cc"],
    deps = [
        ":script_env",
        ":testing",
        "@gtest//:gtest_main",
    ],
)

cc_test(
    name = "math_tests",
    srcs = ["functions/math_tests.cc"],
    deps = [
        ":script_env",
        ":testing",
        "@gtest//:gtest_main",
        "//redux/modules/math:quaternion",
        "//redux/modules/math:vector",
    ],
)

cc_test(
    name = "message_tests",
    srcs = ["functions/message_tests.cc"],
    deps = [
        ":script_env",
        ":testing",
        "@gtest//:gtest_main",
        "//redux/modules/dispatcher:message",
    ],
)

cc_test(
    name = "operators_tests",
    srcs = ["functions/operators_tests.cc"],
    deps = [
        ":script_env",
        ":testing",
        "@gtest//:gtest_main",
    ],
)

cc_test(
    name = "typeof_tests",
    srcs = ["functions/typeof_tests.cc"],
    deps = [
        ":script_env",
        ":testing",
        "@gtest//:gtest_main",
    ],
)
