local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Cubemap Texture',
      features: [utils.hash('Texture')],
      inputs: [{
        name: 'vCubeCoord',
        type: 'Vec3f',
        usage: 'TexCoord',
      }],
      samplers: [{
        name: 'texture_unit_0',
        usage: 'BaseColor',
        type: 'CubeMap',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      main_code: |||
        gl_FragColor *= textureCube(SAMPLER(texture_unit_0), vCubeCoord);
      |||,
    },
    {
      name: 'Cubemap Texture',
      features: [utils.hash('Texture')],
      inputs: [{
        name: 'vCubeCoord',
        type: 'Vec3f',
        usage: 'TexCoord',
      }],
      samplers: [{
        name: 'texture_unit_0',
        usage: 'BaseColor',
        type: 'CubeMap',
      }],
      outputs: [{
        name: 'outColor',
        type: 'Vec4f',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 300,
        max_version: 0,
      }],
      main_code: |||
        outColor *= texture(SAMPLER(texture_unit_0), vCubeCoord);
      |||,
    },
  ],
}
