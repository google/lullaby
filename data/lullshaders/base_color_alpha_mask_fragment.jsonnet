local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Base Color Alpha Mask (Uniform)',
      uniforms: [{
        name: 'AlphaCutoff',
        type: 'Float1',
      }],
      environment: [utils.hash('AlphaCutoff')],
      main_code: |||
        if (base_color.a < UNIFORM(AlphaCutoff)) {
          discard;
        }
      |||,
    },
  ],
}
