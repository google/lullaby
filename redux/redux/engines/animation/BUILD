# Animation engine.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "animation",
    srcs = [
        "animation_clip.cc",
        "animation_engine.cc",
        "motivator/motivator.cc",
        "motivator/rig_motivator.cc",
        "motivator/spline_motivator.cc",
        "motivator/transform_motivator.cc",
        "processor/anim_processor.cc",
        "processor/rig_processor.cc",
        "processor/spline_processor.cc",
        "processor/transform_processor.cc",
    ],
    hdrs = [
        "animation_clip.h",
        "animation_engine.h",
        "animation_playback.h",
        "common.h",
        "motivator/motivator.h",
        "motivator/rig_motivator.h",
        "motivator/spline_motivator.h",
        "motivator/transform_motivator.h",
        "processor/anim_processor.h",
        "processor/index_allocator.h",
        "processor/rig_processor.h",
        "processor/spline_processor.h",
        "processor/transform_processor.h",
    ],
    deps = [
        "@absl//absl/algorithm:container",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/time",
        "//redux/data/asset_defs:anim_asset_def_fbs",
        "//redux/engines/animation/spline:compact_spline",
        "//redux/modules/base:asset_loader",
        "//redux/modules/base:choreographer",
        "//redux/modules/base:data_container",
        "//redux/modules/base:logging",
        "//redux/modules/base:registry",
        "//redux/modules/base:resource_manager",
        "//redux/modules/base:static_registry",
        "//redux/modules/base:typeid",
        "//redux/modules/math:constants",
        "//redux/modules/math:matrix",
        "//redux/modules/math:transform",
        "//redux/modules/math:vector",
    ],
)
