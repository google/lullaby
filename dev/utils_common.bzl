"""Common functions and definitions used by other build rules."""

FLATC_DEFAULT_ARGS = [
    "--no-union-value-namespacing",
    "--gen-name-strings",
]

FLATC_JS_DEFAULT_ARGS = [
    "--bfbs-comments",
    "--gen-name-strings",
    "--goog-js-export",
    "--gen-all",
]

FLATC_DEFAULT_INCLUDES = [
    "//:fbs_includes",
]

FLATC_DEFAULT_INCLUDE_PATHS = [
    "schemas",
]
