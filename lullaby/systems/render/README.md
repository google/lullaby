# RenderSystem

Renders Entities. Buckets and sorts Entities in render passes, manages render
states, performs draw calls.

[TOC]

The render system has many knobs and switches that allow it to be configured,
but still tries to provide a simple interface that remains largely hands-off for
common use cases.

## Backends

The render system has an abstract-like interface, which allows you to choose
from various backends:

* FPL
* Ion
* Mock

The backend is decided in the BUILD file by pointing at one of the corresponding
build targets.  The 'render_system' target is header-only.

## Definition

Like all components, renderables are configured by adding a [RenderDef]
component to an entity's json file.

## Render passes

Render passes determine the overall render state and high level render order.

* Pano: a special use pass for stereoscopic pano images.
* Opaque: intended for opaque 3D models that draw into the depth buffer.
    * Blending disabled, zwrite and ztest enabled.
* Main:
    * Lerp blend based on src alpha, zwrite disabled, ztest enabled.
* OverDraw: a special use pass to draw over everything in the scene.
    * Lerp blend based on src alpha, zwrite and ztest disabled.
* Debug: a special use pass for rendering debug information.
* Invisible: a special use pass whose contents are not drawn.

### Custom render passes

If you need to set a specific render state that is not included in the
predefined render passes or draw things in a very specific order, you can add
entities into custom render passes.

## Meshes

There are two ways to define a mesh: by defining a quad, or by loading an FBX
file.

### Quads

By using a QuadDef, you can create a simple quad mesh, optionally with rounded
corners and auto-generated texture coordinates.  Quads are also affected by a
[deform component]

### FBX files

Through FPL's mesh_pipeline, Lullaby can also load meshes from
[FBX](http://www.autodesk.com/products/fbx/overview) files.  This import path
allows complex meshes to be loaded into the render system.

Meshes loaded from FBX files are not affected by deform components.

## Shaders

Shaders can be loaded into the render system from .fplshader files, which simply
contain the source for both the vertex and fragment shaders.

### Attributes

Vertex attributes follow the FPL naming convention:

* aPosition: vec3
* aNormal: vec3
* aTangent: vec4 (xyz: tangent vector, w: handedness)
* aTexCoord: vec2
* aTexCoordAlt: vec2
* aColor: vec4 (source: normalized ubyte[4])
* aBoneIndices: [i]vec4 (source: ubyte[4])
* aBoneWeights: vec4 (source: normalized ubyte[4])

### Uniforms

Since Lullaby was based on the FPL renderer, by default it uses FPL's uniform
naming convention (hence "model_view_projection" and not
"clip_from_entity_matrix").

The following uniforms are set automatically by the render system:

* color: vec4
  * Normalized rgba.
* uv_bounds: vec4
  * xy: offset, zw: bounds.
* model_view_projection: mat4
  * AKA clip_from_entity_matrix
* model: mat4
  * AKA world_from_entity_matrix
* uIsRightEye: int
  * 1 when drawing right eye, otherwise 0.
* texture_unit_%d: texture
* bone_transforms: array of 3 vec4s (representing mat3x4[])
* camera_pos: vec3
  * Camera position in world space.
* camera_dir: vec3
  * Camera forward direction in world space.

## Textures

Textures can be loaded from various file formats, though only a few are
supported by both backends:

* TGA (FPL, Ion)
* WEBP (FPL, Ion)
* PNG (FPL, Ion)
* JPEG (Ion)
* BMP (Ion)
* PSD (Ion)
* GIF (Ion)
* HDR (Ion)
* PIC (Ion)
* ASTC (FPL)
* PKM (FPL)
* KTX (FPL)

### Atlases

Multiple images can be combined into a single texture atlas using the
fpl_texture_atlas build command.  You can reference individual images by using
an all-caps version of the filename without its extension: eg toggleoff.png
would be referred to as TOGGLEOFF.

## Draw order

Within a render pass, entities can be drawn in a sorted order.  This is most
useful for blend-enabled render passes, but can be configured separately for
each render pass using
[SetSortMode]

By default, the Main render pass is sorted using kSortOrderIncreasing.

### Default sort order

The two kSortOrder sort modes are simply a compare of entities' sort_order key.
By default, this is calculated based on the scene graph so that parents draw
before children, and siblings are drawn in linear order.  This can be tweaked
slightly in the RenderDef by setting the sort_order_offset, which will replace
the sibling order when calculating the sort_order.

The calculation used was chosen to group trees tightly, so that root-level trees
will always draw together, but within a tree the order remains top-down.

```c++
using SortOrder = uint32_t;
const int kNumBitsPerDepth = 4;
const int kRootShift = 8 * sizeof(SortOrder) - kNumBitsPerDepth;
if (parent) {
  default_sort_order = parent_sort_order |
                       (offset << kRootShift - depth * kNumBitsPerDepth);
} else {
  default_sort_order = (offset << kRootShift);
}
```
