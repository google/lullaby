# SDL2 backend for platform abstraction library.

licenses(["notice"])

package(
    default_visibility = ["//redux:visibility"],
)


cc_library(
    name = "sdl2",
    srcs = [
        "sdl2_display.cc",
        "sdl2_keyboard.cc",
        "sdl2_mainloop.cc",
        "sdl2_mouse.cc",
        "sdl2_speaker.cc",
    ],
    hdrs = [
        "sdl2_display.h",
        "sdl2_event_handler.h",
        "sdl2_keyboard.h",
        "sdl2_mainloop.h",
        "sdl2_mouse.h",
        "sdl2_speaker.h",
    ],
    deps = [
        "@sdl2//:sdl2",
        "//redux/modules/math:vector",
        "//redux/engines/platform:device_manager",
        "//redux/engines/platform:display",
        "//redux/engines/platform:mainloop",
        "//redux/engines/platform:mouse",
        "//redux/engines/platform:keyboard",
        "//redux/engines/platform:speaker",
    ] + select({
        "@platforms//os:osx": [
            ":get_native_window_osx",
        ],
        "//conditions:default": [
        ],
    }),
)

objc_library(
    name = "get_native_window_osx",
    srcs = select({
        "@platforms//os:osx": [
            "get_native_window_osx.mm",
        ],
        "//conditions:default": [
        ],
    }),
    deps = select({
        "@platforms//os:osx": [
            # "//third_party/apple_frameworks:Cocoa",
            # "//third_party/apple_frameworks:Metal",
        ],
        "//conditions:default": [
        ],
    }),
)
