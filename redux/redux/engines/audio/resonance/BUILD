licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

cc_library(
    name = "resonance",
    srcs = [
        "audio_asset_manager.cc",
        "audio_asset_stream.cc",
        "audio_planar_data.cc",
        "audio_stream_manager.cc",
        "audio_stream_renderer.cc",
        "resonance_audio_asset.cc",
        "resonance_audio_engine.cc",
        "resonance_sound.cc",
        "resonance_utils.cc",
    ],
    hdrs = [
        "audio_asset_manager.h",
        "audio_asset_stream.h",
        "audio_planar_data.h",
        "audio_source_stream.h",
        "audio_stream_manager.h",
        "audio_stream_renderer.h",
        "resonance_audio_asset.h",
        "resonance_audio_engine.h",
        "resonance_sound.h",
        "resonance_utils.h",
    ],
    deps = [
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/functional:bind_front",
        "@absl//absl/types:span",
        "//redux/engines/audio",
        "//redux/engines/platform:device_manager",
        "//redux/engines/platform:device_profiles",
        "//redux/modules/audio:audio_reader",
        "//redux/modules/audio:enums",
        "//redux/modules/audio:opus_reader",
        "//redux/modules/audio:vorbis_reader",
        "//redux/modules/audio:wav_reader",
        "//redux/modules/base:asset_loader",
        "//redux/modules/base:choreographer",
        "//redux/modules/base:data_reader",
        "//redux/modules/base:logging",
        "//redux/modules/base:registry",
        "//redux/modules/base:static_registry",
        "@resonance//:resonance",
    ],
)

cc_test(
    name = "audio_asset_manager_tests",
    srcs = ["audio_asset_manager_tests.cc"],
    data = [
        "//redux/modules/audio:test_data/speech.wav",
    ],
    deps = [
        ":resonance",
        "@gtest//:gtest_main",
        "//redux/modules/base:asset_loader",
        "//redux/modules/testing",
    ],
)

cc_test(
    name = "audio_asset_stream_tests",
    srcs = ["audio_asset_stream_tests.cc"],
    data = [
        "//redux/modules/audio:test_data/speech.wav",
    ],
    deps = [
        ":resonance",
        "@gtest//:gtest_main",
        "//redux/modules/audio:wav_reader",
        "//redux/modules/testing",
    ],
)

cc_test(
    name = "audio_planar_data_tests",
    srcs = ["audio_planar_data_tests.cc"],
    data = [
        "//redux/modules/audio:test_data/speech.wav",
    ],
    deps = [
        ":resonance",
        "@gtest//:gtest_main",
        "//redux/modules/audio:wav_reader",
        "//redux/modules/testing",
    ],
)
