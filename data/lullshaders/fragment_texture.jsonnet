local utils = import 'third_party/lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Texture',
      features: [utils.hash('Texture')],
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
        usage: 'TexCoord',
      }],
      uniforms: [{
        name: 'texture_unit_0',
        type: 'Sampler2D',
      }, {
        name: 'uv_bounds',
        type: 'Float4',
      }],
      main_code: |||
        vec2 clamped_uvs = clamp(vTexCoord, uv_bounds.xy, uv_bounds.zw);
        gl_FragColor *= texture2D(texture_unit_0, clamped_uvs);
      |||,
    },
  ],
}
