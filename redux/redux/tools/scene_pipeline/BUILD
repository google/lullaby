# Tool for building redux scene files.

licenses(["notice"])

package(
    
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "buffer",
    hdrs = ["buffer.h"],
    deps = [
        ":index",
        "@absl//absl/log:check",
        "@absl//absl/types:span",
    ],
)

cc_test(
    name = "buffer_tests",
    srcs = ["buffer_tests.cc"],
    deps = [
        ":buffer",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "buffer_view",
    hdrs = ["buffer_view.h"],
    deps = [
        ":buffer",
    ],
)

cc_library(
    name = "collider",
    hdrs = ["collider.h"],
    deps = [
        ":buffer_view",
        ":index",
    ],
)

cc_library(
    name = "drawable",
    hdrs = ["drawable.h"],
    deps = [
        ":index",
        ":index_buffer",
        ":material",
        ":vertex_buffer",
    ],
)

cc_library(
    name = "image",
    hdrs = ["image.h"],
    deps = [
        ":buffer_view",
        ":index",
    ],
)

cc_library(
    name = "index",
    hdrs = ["index.h"],
    deps = [
        "@absl//absl/log:check",
    ],
)

cc_test(
    name = "index_tests",
    srcs = ["index_tests.cc"],
    deps = [
        ":index",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "index_buffer",
    hdrs = ["index_buffer.h"],
    deps = [
        ":buffer_view",
        ":type_id",
    ],
)

cc_library(
    name = "material",
    hdrs = ["material.h"],
    deps = [
        ":index",
        ":types",
        ":variant",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/strings:string_view",
        "@absl//absl/types:span",
    ],
)

cc_library(
    name = "model",
    hdrs = ["model.h"],
    deps = [
        ":collider",
        ":drawable",
        ":index",
        ":types",
    ],
)

cc_library(
    name = "safe_vector",
    hdrs = ["safe_vector.h"],
    deps = [
        ":index",
        "@absl//absl/log:check",
    ],
)

cc_library(
    name = "sampler",
    hdrs = ["sampler.h"],
    deps = [
        ":image",
        ":types",
    ],
)

cc_library(
    name = "scene",
    hdrs = ["scene.h"],
    deps = [
        ":buffer",
        ":buffer_view",
        ":collider",
        ":drawable",
        ":image",
        ":material",
        ":model",
        ":safe_vector",
    ],
)

cc_library(
    name = "type_id",
    hdrs = ["type_id.h"],
)

cc_test(
    name = "type_id_tests",
    srcs = ["type_id_tests.cc"],
    deps = [
        ":type_id",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "types",
    hdrs = ["types.h"],
)

cc_library(
    name = "variant",
    hdrs = ["variant.h"],
    deps = [
        ":type_id",
        "@absl//absl/types:span",
    ],
)

cc_test(
    name = "variant_tests",
    srcs = ["variant_tests.cc"],
    deps = [
        ":type_id",
        ":variant",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "vertex_buffer",
    hdrs = ["vertex_buffer.h"],
    deps = [
        ":buffer_view",
        ":type_id",
        ":types",
    ],
)
