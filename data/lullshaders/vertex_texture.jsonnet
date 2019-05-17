local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Texture UVs',
      features: [utils.hash('Texture')],
      inputs: [{
        name: 'aTexCoord',
        type: 'Vec2f',
        usage: 'TexCoord',
      }],
      outputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }],
      uniforms: [{
        name: 'uv_bounds',
        type: 'Float4',
      }],
      main_code: |||
        vTexCoord = uv_bounds.xy + aTexCoord * uv_bounds.zw;
      |||,
    },
    {
      name: 'Texture UV1s',
      features: [utils.hash('Texture1')],
      inputs: [{
        name: 'aTexCoordAlt',
        type: 'Vec2f',
        usage: 'TexCoord',
      }],
      outputs: [{
        name: 'vTexCoordAlt',
        type: 'Vec2f',
      }],
      main_code: |||
        vTexCoordAlt = aTexCoordAlt;
      |||,
    },
  ],
}
