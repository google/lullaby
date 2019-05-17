local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Emissive Color (Black)',
      code: |||
        vec3 emissive;
      |||,
      main_code: |||
        emissive = vec3(0);
      |||,
    },
    {
      name: 'Emissive Color (Uniform)',
      uniforms: [{
        name: 'Emissive',
        type: 'Float3',
        values: [0.0, 0.0, 0.0],
      }, {
        name: 'color',
        type: 'Float4',
        values: [1.0, 1.0, 1.0, 1.0],
      }],
      main_code: |||
        emissive = UNIFORM(Emissive) * UNIFORM(color).rgb;
      |||,
    },
    {
      name: 'Emissive Color (Texture)',
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }],
      samplers: [{
        name: 'EmissiveTexture',
        usage: 'Emissive',
        type: 'Standard2d',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      main_code: |||
        emissive *=
            RemoveGamma(texture2D(SAMPLER(EmissiveTexture), vTexCoord).rgb);
      |||,
    },
    {
      name: 'Emissive Color (Commit)',
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      main_code: |||
        gl_FragColor.rgb += emissive;
      |||,
    },
    {
      name: 'Emissive Color (Texture)',
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
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
      samplers: [{
        name: 'EmissiveTexture',
        usage: 'Emissive',
        type: 'Standard2d',
      }],
      main_code: |||
        emissive *=
            RemoveGamma(texture(SAMPLER(EmissiveTexture), vTexCoord).rgb);
      |||,
    },
    {
      name: 'Emissive Color (Commit)',
      versions: [{
        lang: 'GL_Compat',
        min_version: 300,
        max_version: 0,
      }],
      main_code: |||
        outColor.rgb += emissive;
      |||,
    },
  ],
}
