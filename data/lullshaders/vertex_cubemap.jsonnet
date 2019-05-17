local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Cubemap TexCoords',
      inputs: [{
        name: 'aNormal',
        type: 'Vec3f',
        usage: 'Normal',
      }],
      outputs: [{
        name: 'vCubeCoord',
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
        vCubeCoord = UNIFORM(mat_normal) * aNormal;
      |||,
    },
  ],
}
