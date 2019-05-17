# `TextSystem`


Manages the rendering of i18n strings.

## Backends

### FlatUI

FlatUI supports our full feature set: i18n, line wrapping, hyphenation, system
fonts, ellipsis, HTML parsing.

#### System fonts

System fonts are most often used to fill in glyphs that are unsupported by the
primary font.

A few things are necessary to load system fonts on Android:

*   App must have storage read access.
    *   `<uses-permission
        android:name="android.permission.READ_EXTERNAL_STORAGE"/>`
*   File loader must support absolute paths.
    *   FlatUI needs to read from /system/etc/fonts.xml and system/fonts/.
    *   The default file loader supports absolute paths.
*   App must have a JavaVM.
    *   Use the fplbase::AndroidSetJavaVM function.

