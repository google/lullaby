local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Reinhard Tonemapping',
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      main_code: |||
        gl_FragColor.rgb = gl_FragColor.rgb / (gl_FragColor.rgb + 1.0);
      |||,
    },
    {
      name: 'Reinhard Tonemapping',
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
        outColor.rgb = outColor.rgb / (outColor.rgb + 1.0);
      |||,
    },
  ],
}
