load("@//dev:build_entity.bzl", "build_entity_bin")
load("@//dev:build_model.bzl", "build_model")

licenses(["notice"])  # Apache 2.0

build_entity_bin(
    name = "entities",
    srcs = glob(["*.jsonnet"]),
    jsonnet_deps = ["//data:jsonnet"],
    schema = "//lullaby/examples/hello_model:entity_schema",
)

build_model(
    name = "models",
    srcs = glob([
        "*.fbx",
    ]),
    textures = glob([
        "*.png",
    ]),
)

Fileset(
    name = "data",
    out = "assets",
    entries = [
        FilesetEntry(
            files = [
                ":entities",
                ":models",
            ],
            strip_prefix = "",
        ),
        FilesetEntry(
            srcdir = "//data:BUILD",
            files = [
                "//data:pbr_shading_model",
            ],
            strip_prefix = "",
        ),
    ],
    visibility = ["//visibility:public"],
)
