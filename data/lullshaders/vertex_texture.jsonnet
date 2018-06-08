local utils = import 'third_party/lullaby/data/jsonnet/utils.jsonnet';
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
  ],
}
