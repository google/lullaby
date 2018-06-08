local utils = import 'third_party/lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: "View direction",
      inputs: [{
        name: 'aPosition',
        type: 'Vec4f',
        usage: 'Position',
      }],
      outputs: [{
        name: 'vViewDirection',
        type: 'Vec3f',
      }],
      uniforms: [{
        name: 'model',
        type: 'Float4x4',
      }, {
        name: 'camera_pos',
        type: 'Float3',
      }],
      main_code: |||
        vec4 position_homogeneous = model * aPosition;
        vec3 position = position_homogeneous.xyz / position_homogeneous.w;
        vViewDirection = camera_pos - position;
      |||,
    },
  ],
}
