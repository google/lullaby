The anim_pipeline is a tool for building redux animation files.

Animation files contains a series of curve definitions that describe the
movement of values over time. Each curve is applied to a single dimension of
a bone in a skeleton which, combined, is used for skeletal animation.

The pipeline converts data created from digitial content creation (dcc) tools
such as Maya FBX files or Khronos GLTF files into .rxanim files that can be
consumed efficiently by the redux runtime.
