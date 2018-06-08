load("@//dev:build_entity.bzl", "build_entity_bin")
load("@//dev:build_model.bzl", "build_model")

licenses(["notice"])  # Apache 2.0

build_entity_bin(
    name = "entities",
    srcs = glob(["*.jsonnet"]),
    jsonnet_deps = ["//third_party/lullaby/data:jsonnet"],
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
            srcdir = "//third_party/lullaby/data:BUILD",
            files = [
                "//third_party/lullaby/data:unlit_shader",
            ],
            strip_prefix = "",
        ),
    ],
    visibility = ["//visibility:public"],
)
