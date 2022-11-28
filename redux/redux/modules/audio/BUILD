# Common audio-related types and functions.

load(
    "//redux/tools:flatbuffer_cc_library.bzl",
    "flatbuffer_cc_library",
)

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)

exports_files(
    srcs = glob(["test_data/*"]),
)

flatbuffer_cc_library(
    name = "audio_enums_fbs",
    srcs = ["audio_enums.fbs"],
)

cc_library(
    name = "audio_reader",
    hdrs = ["audio_reader.h"],
    deps = ["@absl//absl/types:span"],
)

cc_library(
    name = "enums",
    hdrs = ["enums.h"],
    deps = [
        ":audio_enums_fbs",
        "//redux/modules/base:typeid",
    ],
)

cc_library(
    name = "opus_reader",
    srcs = ["opus_reader.cc"],
    hdrs = ["opus_reader.h"],
    deps = [
        ":audio_reader",
        "@libopusfile//:libopusfile",
        "//redux/modules/base:data_reader",
        "//redux/modules/base:logging",
    ],
)

cc_test(
    name = "opus_reader_tests",
    srcs = ["opus_reader_tests.cc"],
    data = [
        "//redux/modules/audio:test_data/small.opus",
        "//redux/modules/audio:test_data/speech.opus",
        "//redux/modules/audio:test_data/speech.wav",
    ],
    deps = [
        ":opus_reader",
        "@gtest//:gtest_main",
        "//redux/modules/testing",
    ],
)

cc_library(
    name = "vorbis_reader",
    srcs = ["vorbis_reader.cc"],
    hdrs = ["vorbis_reader.h"],
    deps = [
        ":audio_reader",
        "@libvorbis//:libvorbis",
        "//redux/modules/base:data_reader",
        "//redux/modules/base:logging",
    ],
)

cc_test(
    name = "vorbis_reader_tests",
    srcs = ["vorbis_reader_tests.cc"],
    data = [
        "//redux/modules/audio:test_data/small.ogg",
        "//redux/modules/audio:test_data/speech.ogg",
        "//redux/modules/audio:test_data/speech.wav",
    ],
    deps = [
        ":vorbis_reader",
        "@gtest//:gtest_main",
        "//redux/modules/testing",
    ],
)

cc_library(
    name = "wav_reader",
    srcs = ["wav_reader.cc"],
    hdrs = ["wav_reader.h"],
    deps = [
        ":audio_reader",
        "//redux/modules/base:data_reader",
        "//redux/modules/base:logging",
    ],
)

cc_test(
    name = "wav_reader_tests",
    srcs = ["wav_reader_tests.cc"],
    data = [
        "//redux/modules/audio:test_data/ambisonic.wav",
        "//redux/modules/audio:test_data/speech.ogg",
        "//redux/modules/audio:test_data/speech.wav",
    ],
    deps = [
        ":wav_reader",
        "@gtest//:gtest_main",
        "//redux/modules/testing",
    ],
)
