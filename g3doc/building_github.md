## Building

-   Use flatbuffers to generate header files for all the .fbs files in the
    schemas folder.
    - ex:
```flatc
     -c
     --no-union-value-namespacing
     --gen-name-strings
     -I schemas
     -o src/lullaby/generated
     schemas/lull/[filename].fbs
```
-   Build all the files with the following exceptions:
    -   For android, remove
        src/lullaby/systems/render/detail/port/default/gpu_profiler.cc
    -   For all other platforms, remove
        src/lullaby/systems/render/detail/port/android/gpu_profiler.cc

### Dependencies

-   [ion/base](https://github.com/google/ion)
-   [mathfu](https://github.com/google/mathfu)
-   [motive](https://github.com/google/motive)
-   [flatui](https://github.com/google/flatui)
-   [fplbase:stdlib](https://github.com/google/fplbase)
-   [flatbuffers](https://github.com/google/flatbuffers)

Transitive dependencies include:

-   freetype 2.6.1
-   gumbo-parser
-   harfbuzz
-   libunibreak
-   libwebp:webp_decode
-   stblib
-   vectorial
-   opengl
