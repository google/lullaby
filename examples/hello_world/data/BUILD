load("//third_party/lullaby:build_defs.bzl", "lullaby_generate_entities")
load("//third_party/lullaby/data:build_defs/copy_files.bzl", "copy_files")

licenses(["notice"])  # Apache 2.0

lullaby_generate_entities(
    name = "entities",
    srcs = glob(["*.json"]),
    schema = "//third_party/lullaby/examples/hello_world:entity_schema",
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
            srcdir = "//third_party/lullaby/data:BUILD",
            files = [
                "//third_party/lullaby/data:text_shader",
            ],
            strip_prefix = "",
        ),
    ],
    visibility = ["//third_party/lullaby/examples/hello_world:__pkg__"],
)
