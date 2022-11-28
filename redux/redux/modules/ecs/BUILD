# Entity-Component-System framework.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "entity",
    hdrs = ["entity.h"],
    deps = ["//redux/modules/base:typeid"],
)

cc_library(
    name = "ecs",
    srcs = [
        "blueprint_factory.cc",
        "entity_factory.cc",
    ],
    hdrs = [
        "blueprint.h",
        "blueprint_factory.h",
        "component_serializer.h",
        "entity_factory.h",
        "system.h",
    ],
    deps = [
        ":entity",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/hash",
        "@absl//absl/status",
        "//redux/engines/script",
        "//redux/engines/script/redux:script_env",
        "//redux/modules/base:asset_loader",
        "//redux/modules/base:bits",
        "//redux/modules/base:function_traits",
        "//redux/modules/base:logging",
        "//redux/modules/base:registry",
        "//redux/modules/base:resource_manager",
        "//redux/modules/base:serialize",
        "//redux/modules/base:static_registry",
        "//redux/modules/base:typeid",
        "//redux/modules/datafile:datafile_parser",
        "//redux/modules/var",
        "//redux/modules/var:var_convert",
    ],
)

cc_test(
    name = "blueprint_factory_tests",
    srcs = ["blueprint_factory_tests.cc"],
    deps = [
        ":ecs",
        "@gtest//:gtest_main",
        "//redux/engines/script/redux",
        "//redux/engines/script/redux:script_env",
        "//redux/modules/var",
    ],
)

cc_test(
    name = "entity_factory_tests",
    srcs = ["entity_factory_tests.cc"],
    deps = [
        ":ecs",
        "@gtest//:gtest_main",
        "//redux/engines/script/redux",
    ],
)
