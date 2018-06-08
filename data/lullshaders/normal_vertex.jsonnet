local utils = import 'third_party/lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: "Normal",
      inputs: [{
        name: 'aNormal',
        type: 'Vec3f',
        usage: 'Normal',
      }],
      outputs: [{
        name: 'vNormal',
        type: 'Vec3f',
        usage: 'Normal',
      }],
      uniforms: [
        {
          name: 'mat_normal',
          type: 'Float3x3',
        },
      ],
      main_code: |||
        vNormal = mat_normal * aNormal;
      |||,
    },
  ],
}
