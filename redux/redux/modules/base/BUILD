# Collection of stand-alone libraries that are used by many other parts of redux.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "asset_loader",
    srcs = ["asset_loader.cc"],
    hdrs = ["asset_loader.h"],
    deps = [
        ":async_processor",
        ":choreographer",
        ":data_builder",
        ":data_container",
        ":data_reader",
        ":logging",
        ":registry",
        "@absl//absl/status:statusor",
    ],
)

cc_test(
    name = "asset_loader_tests",
    srcs = ["asset_loader_tests.cc"],
    deps = [
        ":asset_loader",
        ":data_builder",
        "@gtest//:gtest_main",
        "@absl//absl/functional:bind_front",
    ],
)

cc_library(
    name = "async_processor",
    hdrs = ["async_processor.h"],
    deps = [
        ":logging",
        ":thread_safe_deque",
    ],
)

cc_test(
    name = "async_processor_tests",
    srcs = ["async_processor_tests.cc"],
    deps = [
        ":async_processor",
        "@gtest//:gtest_main",
        "@absl//absl/synchronization",
    ],
)

cc_library(
    name = "bits",
    hdrs = ["bits.h"],
)

cc_test(
    name = "bits_tests",
    srcs = ["bits_tests.cc"],
    deps = [
        ":bits",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "choreographer",
    srcs = ["choreographer.cc"],
    hdrs = ["choreographer.h"],
    deps = [
        ":dependency_graph",
        ":logging",
        ":registry",
        ":typeid",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/time",
    ],
)

cc_test(
    name = "choreographer_tests",
    srcs = ["choreographer_tests.cc"],
    deps = [
        ":choreographer",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "data_buffer",
    hdrs = ["data_buffer.h"],
    deps = [
        ":typeid",
        "@absl//absl/types:span",
    ],
)

cc_test(
    name = "data_buffer_tests",
    srcs = ["data_buffer_tests.cc"],
    deps = [
        ":data_buffer",
        "@gtest//:gtest_main",
        "@absl//absl/types:span",
    ],
)

cc_library(
    name = "data_builder",
    hdrs = ["data_builder.h"],
    deps = [
        ":data_container",
        ":logging",
        "@absl//absl/types:span",
    ],
)

cc_test(
    name = "data_builder_tests",
    srcs = ["data_builder_tests.cc"],
    deps = [
        ":data_builder",
        ":data_container",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "data_container",
    hdrs = ["data_container.h"],
    deps = [
        ":logging",
        "@absl//absl/types:span",
    ],
)

cc_test(
    name = "data_container_tests",
    srcs = ["data_container_tests.cc"],
    deps = [
        ":data_container",
        "@gtest//:gtest_main",
        "@absl//absl/types:span",
    ],
)

cc_library(
    name = "data_reader",
    srcs = ["data_reader.cc"],
    hdrs = ["data_reader.h"],
    deps = [
        ":logging",
        "@absl//absl/types:span",
    ],
)

cc_test(
    name = "data_reader_tests",
    srcs = ["data_reader_tests.cc"],
    deps = [
        ":data_reader",
        "@gtest//:gtest_main",
        "@absl//absl/types:span",
    ],
)

cc_library(
    name = "dependency_graph",
    hdrs = ["dependency_graph.h"],
    deps = [
        ":logging",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:flat_hash_set",
    ],
)

cc_test(
    name = "dependency_graph_tests",
    srcs = ["dependency_graph_tests.cc"],
    deps = [
        ":dependency_graph",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "filepath",
    srcs = ["filepath.cc"],
    hdrs = ["filepath.h"],
)

cc_test(
    name = "filepath_tests",
    srcs = ["filepath_tests.cc"],
    deps = [
        ":filepath",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "function_traits",
    hdrs = ["function_traits.h"],
)

cc_test(
    name = "function_traits_tests",
    srcs = ["function_traits_tests.cc"],
    deps = [
        ":function_traits",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "hash",
    srcs = ["hash.cc"],
    hdrs = ["hash.h"],
)

cc_test(
    name = "hash_tests",
    srcs = ["hash_tests.cc"],
    deps = [
        ":hash",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "logging",
    hdrs = ["logging.h"],
    deps = [
        "@absl//absl/log",
        "@absl//absl/log:check",
    ],
)

cc_library(
    name = "registry",
    srcs = ["registry.cc"],
    hdrs = ["registry.h"],
    deps = [
        ":dependency_graph",
        ":logging",
        ":typeid",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/synchronization",
    ],
)

cc_test(
    name = "registry_tests",
    srcs = ["registry_tests.cc"],
    deps = [
        ":registry",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "resource_manager",
    hdrs = ["resource_manager.h"],
    deps = [
        ":hash",
        ":logging",
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_test(
    name = "resource_manager_tests",
    srcs = ["resource_manager_tests.cc"],
    deps = [
        ":resource_manager",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "serialize",
    hdrs = [
        "archiver.h",
        "serialize.h",
    ],
    deps = [
        ":hash",
    ],
)

cc_test(
    name = "serialize_tests",
    srcs = ["serialize_tests.cc"],
    deps = [
        ":hash",
        ":serialize",
        "@gtest//:gtest_main",
        "@absl//absl/types:any",
    ],
)

cc_library(
    name = "serialize_traits",
    hdrs = ["serialize_traits.h"],
    deps = [
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_test(
    name = "serialize_traits_tests",
    srcs = ["serialize_traits_tests.cc"],
    deps = [
        ":serialize_traits",
        "@gtest//:gtest_main",
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_library(
    name = "static_registry",
    srcs = ["static_registry.cc"],
    hdrs = ["static_registry.h"],
    deps = [
        ":registry",
        "@absl//absl/base:core_headers",
    ],
)

cc_library(
    name = "thread_safe_deque",
    hdrs = ["thread_safe_deque.h"],
    deps = [
        "@absl//absl/base:core_headers",
        "@absl//absl/synchronization",
    ],
)

cc_test(
    name = "thread_safe_deque_tests",
    srcs = ["thread_safe_deque_tests.cc"],
    deps = [
        ":thread_safe_deque",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "typed_ptr",
    hdrs = ["typed_ptr.h"],
    deps = [
        ":typeid",
    ],
)

cc_test(
    name = "typed_ptr_tests",
    srcs = ["typed_ptr_tests.cc"],
    deps = [
        ":typed_ptr",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "typeid",
    hdrs = ["typeid.h"],
    deps = [
        ":hash",
    ],
)

cc_library(
    name = "data_table",
    hdrs = [
        "data_table.h",
        "ref_tuple.h",
    ],
    deps = [
        ":logging",
        "@absl//absl/container:flat_hash_map",
    ],
)

cc_test(
    name = "data_table_tests",
    srcs = [
        "data_table_tests.cc",
        "ref_tuple_tests.cc",
    ],
    deps = [
        ":data_table",
        "@gtest//:gtest_main",
    ],
)
