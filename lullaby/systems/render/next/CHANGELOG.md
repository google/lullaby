This file contains a list of changes that apps will need to make in order to
switch to RenderSystemNext.

### SubmitRenderData/BeginRendering/EndRendering

Clients need to call SubmitRenderData on the SimThread.  Then, on the
RenderThread, they should call:

BeginFrame
BeginRendering
Render
EndRendering
EndFrame


### RenderPass

The RenderPass enum is no longer valid.  Clients should now create/manage
passes by HashValue.


### TextDef, QuadDef, PanoDef

You can no longer create Text, Quad, or Panos using the RenderDef.  Instead, the
corresponding Systems should be used to create those Entities. Specifically:
* TextSystem for creating text.
* PanoSystem for creating panos.
* NinePatchSystem for creating quads.

