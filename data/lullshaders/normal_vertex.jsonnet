local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
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
        vNormal = UNIFORM(mat_normal) * aNormal;
      |||,
    },
    {
      name: "Normal",
      inputs: [{
        name: 'aOrientation',
        type: 'Vec4f',
        usage: 'Orientation',
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
        vec3 n = vec3(0.0,  0.0,  1.0) +
                 vec3(2.0, -2.0, -2.0) * aOrientation.x * aOrientation.zwx +
                 vec3(2.0,  2.0, -2.0) * aOrientation.y * aOrientation.wzy;
        vNormal = UNIFORM(mat_normal) * n;
      |||,
    },
  ],
}
