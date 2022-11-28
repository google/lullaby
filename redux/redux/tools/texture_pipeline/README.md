The texture_pipeline is a tool for building texture files.

The texture pipeline can be used to convert images into formats that are
supported by the redux runtime: PNG, WEBP, ETC2 and ASTC. Note that ETC2 and
ASTC are hardware formats, so only runtimes for specific platforms can use these
formats.

Images can also be packed into KTX container formats. This is useful for
bundling multiple images (eg. mipmaps or cubemaps) into a single file.
