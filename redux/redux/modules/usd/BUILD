# Utilities for dealing with USD files.
load(
    "//redux/modules/usd:build_defs.bzl",
    "copy_plugins_and_schemas",
)

licenses(["notice"])

package(
    
    default_visibility = ["//redux:visibility"],
)

exports_files(["redux/plugInfo.json"])

copy_plugins_and_schemas(
    name = "usd_plugins",
    srcs = [
        "@usd//:ar_resources_datafiles",
        "@usd//:sdf_resources_datafiles",
        "@usd//:usdGeom_resources_datafiles",
        "@usd//:usdShade_resources_datafiles",
        "@usd//:usdSkel_resources_datafiles",
        "@usd//:usd_resources_datafiles",
    ],
    outdir = "usd",
)

cc_library(
    name = "init_usd_plugins",
    srcs = ["init_usd_plugins.cc"],
    hdrs = ["init_usd_plugins.h"],
    deps = [
        ":usd_asset_resolver",
        "@absl//absl/log:check",
        "//redux/modules/base:registry",
        "@usd//:ar",
        "@usd//:plug",
    ],
)

cc_library(
    name = "usd_asset_resolver",
    srcs = ["usd_asset_resolver.cc"],
    hdrs = ["usd_asset_resolver.h"],
    deps = [
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/log",
        "@absl//absl/log:check",
        "@absl//absl/synchronization",
        "//redux/modules/base:asset_loader",
        "//redux/modules/base:data_container",
        "//redux/modules/base:registry",
        "@usd//:ar",
        "@usd//:arch",
        "@usd//:vt",
    ],
)
