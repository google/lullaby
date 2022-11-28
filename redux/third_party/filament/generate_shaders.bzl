
def generate_shaders(name):
    gendir = "generated"

    native.genrule(
        name = name + "_" + gendir,
        srcs = native.glob([
            "shaders/src/*.cs",
            "shaders/src/*.vs",
            "shaders/src/*.fs",
            "shaders/src/*.glsl",
        ]),
        outs = [
            gendir + "/shaders.bin",
            gendir + "/shaders.h",
            gendir + "/shaders.c",
            gendir + "/shaders.S",
            gendir + "/shaders.apple.S",
        ],
        tools = [":resgen"],
        cmd = "DEPLOY=`dirname $(location " + gendir + "/shaders.bin)` && " +
              "$(location :resgen) -kcp shaders --text --deploy=$$DEPLOY $(SRCS)",
    )

    native.cc_library(
        name = name,
        srcs = [gendir + "/shaders.c"],
        includes = [gendir],
        textual_hdrs = [gendir + "/shaders.h"],
    )
