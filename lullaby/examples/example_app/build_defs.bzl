"""BUILD extensions for creating lullaby example apps."""


def lullaby_example(
        name,
        ports = ["desktop"],
        srcs = [],
        hdrs = [],
        deps = [],
        data = []):
    """Generates lullaby example binary targets.

    Args:
      name: The "base" name of the app binary targets. The actual name of the
            targets will be of the form "<basename>_<port>".
      ports: List of "ports" to target.
      srcs: The C++ source files of the app.
      hdrs: The C++ headers files of the app.
      deps: The C++ dependencies for the app.
      data: The list of assets to be packaged with app.
    """

    # Generate the native cc library containing the specified srcs.
    lib_name = "%s_lib" % name
    native.cc_library(
        name = lib_name,
        srcs = srcs,
        hdrs = hdrs,
        deps = deps,
        alwayslink = 1,
    )


    if "desktop" in ports:
        # Wrap the library in a binary along with the sdl2 main desktop library.
        native.cc_binary(
            name = "%s_desktop" % name,
            data = data,
            deps = [
                ":%s" % lib_name,
                "//lullaby/examples/example_app:sdl2_main",
            ],
        )


