load("@//dev:build_entity.bzl", "build_entity_bin")
load("@//dev:copy_files.bzl", "copy_files")

licenses(["notice"])  # Apache 2.0

build_entity_bin(
    name = "entities",
    srcs = glob(["*.json"]),
    schema = "//lullaby/examples/hello_world:entity_schema",
)

copy_files(
    name = "fonts",
    srcs = ["//third_party/webfonts/apache:Roboto-Regular.ttf"],
    outs = ["Roboto-Regular.ttf"],
)

Fileset(
    name = "data",
    out = "assets",
    entries = [
        FilesetEntry(
            files = [
                ":entities",
                ":fonts",
            ],
            strip_prefix = "",
        ),
        FilesetEntry(
            srcdir = "//data:BUILD",
            files = [
                "//data:text_shader",
            ],
            strip_prefix = "",
        ),
    ],
    visibility = ["//lullaby/examples/hello_world:__pkg__"],
)
