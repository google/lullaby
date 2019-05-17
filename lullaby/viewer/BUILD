load(
    "@//dev:generate_entity_schema.bzl",
    "generate_entity_schema",
)

licenses(["notice"])  # Apache 2.0

exports_files([
    "LICENSE",
    "fontawesome-webfont.ttf",
])

generate_entity_schema(
    src = "entity.fbs",
    library_name = "entity_library",
    schema_name = "entity_schema.fbs",
)

Fileset(
    name = "data",
    out = "assets",
    entries = [
        FilesetEntry(
            files = [
                ":entity_schema.fbs",
            ],
        ),
        FilesetEntry(
            files = [
                "fontawesome-webfont.ttf",
            ],
        ),
        FilesetEntry(
            files = [
                "data/brdf_lookup.png",
                "data/forest_diff_cubemap.png",
                "data/forest_spec_cubemap.png",
            ],
        ),
        FilesetEntry(
            srcdir = "//data:BUILD",
            files = [
                "//data:jsonnet_srcs",
                "//data:shader_srcs",
            ],
        ),
        FilesetEntry(
            srcdir = "//:BUILD",
            files = [
                "//:schema_srcs",
            ],
        ),
    ],
)

cc_library(
    name = "imgui_bridge",
    srcs = [
        "src/imgui_bridge.cc",
    ],
    hdrs = [
        "src/imgui_bridge.h",
    ],
    copts = [
        "-Ithird_party/GL/gl/include/",
        "-UNOGDI",
    ],
    defines = select({
        "//:target_os_windows": [],
        "//conditions:default": ["GL_GLEXT_PROTOTYPES"],
    }),
    visibility = ["//visibility:public"],
    deps = [
        "@sdl2//:sdl2",
        "//third_party/dear_imgui",
        "@fplbase//:glplatform",
        "//lullaby/util:logging",
    ],
)

cc_library(
    name = "lib",
    srcs = [
        "src/builders/build_blueprint.cc",
        "src/builders/build_model.cc",
        "src/builders/build_rig_animation.cc",
        "src/builders/build_shader.cc",
        "src/builders/build_shading_model.cc",
        "src/builders/build_stategraph.cc",
        "src/builders/flatbuffers.cc",
        "src/builders/jsonnet.cc",
        "src/file_manager.cc",
        "src/jsonnet_writer.cc",
        "src/main.cc",
        "src/viewer.cc",
        "src/widgets/build_blueprint_popup.cc",
        "src/widgets/build_model_popup.cc",
        "src/widgets/build_shader_popup.cc",
        "src/widgets/console.cc",
        "src/widgets/entity_editor.cc",
        "src/widgets/file_dialog.cc",
        "src/widgets/preview_window.cc",
        "src/window.cc",
    ],
    hdrs = [
        "src/builders/build_blueprint.h",
        "src/builders/build_model.h",
        "src/builders/build_rig_animation.h",
        "src/builders/build_shader.h",
        "src/builders/build_shading_model.h",
        "src/builders/build_stategraph.h",
        "src/builders/flatbuffers.h",
        "src/builders/jsonnet.h",
        "src/file_manager.h",
        "src/jsonnet_writer.h",
        "src/viewer.h",
        "src/widgets/build_blueprint_popup.h",
        "src/widgets/build_model_popup.h",
        "src/widgets/build_shader_popup.h",
        "src/widgets/console.h",
        "src/widgets/entity_editor.h",
        "src/widgets/file_dialog.h",
        "src/widgets/preview_window.h",
        "src/window.h",
    ],
    deps = [
        ":entity_library",
        ":imgui_bridge",
        "//ion/base:ionbase",
        "@sdl2//:sdl2",
        "//third_party/dear_imgui",
        "@flatbuffers//:flatbuffers",
        "@flatbuffers//:flatc_library",
        "@fplbase//:fplbase",
        "@fplbase//:glplatform",
        "//third_party/fplbase/shader_pipeline:shader_pipeline_lib",
        "@jsonnet//core:libjsonnet",
        "//lullaby/modules/animation_channels:render_channels",
        "//lullaby/modules/dispatcher",
        "//lullaby/modules/ecs",
        "//lullaby/modules/file",
        "//lullaby/modules/input",
        "//lullaby/modules/lullscript",
        "//lullaby/modules/script",
        "//lullaby/systems/animation",
        "//lullaby/systems/audio",
        "//lullaby/systems/collision",
        "//lullaby/systems/datastore",
        "//lullaby/systems/dispatcher",
        "//lullaby/systems/light",
        "//lullaby/systems/model_asset",
        "//lullaby/systems/name",
        "//lullaby/systems/physics",
        "//lullaby/systems/render:next",
        "//lullaby/systems/rig",
        "//lullaby/systems/script",
        "//lullaby/systems/stategraph",
        "//lullaby/systems/text:flatui",
        "//lullaby/systems/transform",
        "//lullaby/util:arg_parser",
        "//lullaby/util:clock",
        "//lullaby/util:color",
        "//lullaby/util:common_types",
        "//lullaby/util:entity",
        "//lullaby/util:filename",
        "//lullaby/util:logging",
        "//lullaby/util:make_unique",
        "//lullaby/util:math",
        "//lullaby/util:registry",
        "//lullaby/util:string_view",
        "//lullaby/util:variant",
        "//lullaby/tools/common:file_utils",
        "//lullaby/tools/common:jsonnet_utils",
        "//lullaby/tools/model_pipeline:import_asset",
        "//lullaby/tools/model_pipeline:import_fbx",
        "//lullaby/tools/model_pipeline:model_pipeline_lib",
        "//lullaby/tools/shader_pipeline:shader_pipeline_lib",
        "@motive//:motive:anim_pipeline_lib",
        "//third_party/qt:v5_4_1_native",
        "@rapidjson//:rapidjson",
    ],
)

cc_binary(
    name = "viewer",
    srcs = [
        "src/main.cc",
    ],
    data = [
        ":data",
    ],
    deps = [
        ":lib",
        "@flatbuffers//:flatbuffers",
        "//lullaby/util:arg_parser",
    ],
)
