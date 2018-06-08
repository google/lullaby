local utils = import 'third_party/lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Normal',
      features: [utils.hash('Light')],
      inputs: [{
        name: 'aPosition',
        type: 'Vec4f',
        usage: 'Position',
      }, {
        name: 'aNormal',
        type: 'Vec3f',
        usage: 'Normal',
      }],
      outputs: [{
        name: 'vNormal',
        type: 'Vec3f',
      }, {
        name: 'vVertPos',
        type: 'Vec3f',
      }],
      uniforms: [
        {
          name: 'model',
          type: 'Float4x4',
        },
        {
          name: 'mat_normal',
          type: 'Float3x3',
        },
      ],
      main_code: |||
        vNormal = mat_normal * aNormal;
        vVertPos = (model * aPosition).xyz;
      |||,
    },
  ],
}
