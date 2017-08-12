# `NinePatchSystem`

*   Contact: aaronbdwyer
*   Status: **Ready**

Provides a component for rendering nine patches. Given the dimensions of a quad
to fill, the original (unaltered) size of the nine patch, and the locations of
the slices, a mesh is generated with appropriate vertex locations and texture
coordinates. A RenderDef should be paired with this component to actually get it
to draw. No nine patch specific shader is needed--texture coordinates will be
set up so that unstretchable and stretchable regions behave appropriately.
