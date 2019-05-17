local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Texture',
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
        usage: 'TexCoord',
      }],
      uniforms: [{
        name: 'uv_bounds',
        type: 'Float4',
      }],
      samplers: [{
        name: 'texture_unit_0',
        usage: 'BaseColor',
        type: 'Standard2d',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      main_code: |||
        vec2 clamped_uvs =
            clamp(vTexCoord, UNIFORM(uv_bounds).xy, UNIFORM(uv_bounds).zw);
        gl_FragColor *= texture2D(SAMPLER(texture_unit_0), clamped_uvs);
      |||,
    },
    {
      name: 'Texture',
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
        usage: 'TexCoord',
      }],
      uniforms: [{
        name: 'uv_bounds',
        type: 'Float4',
      }],
      samplers: [{
        name: 'texture_unit_0',
        usage: 'BaseColor',
        type: 'Standard2d',
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
        vec2 clamped_uvs =
            clamp(vTexCoord, UNIFORM(uv_bounds).xy, UNIFORM(uv_bounds).zw);
        outColor *= texture(SAMPLER(texture_unit_0), clamped_uvs);
      |||,
    },
  ],
}
