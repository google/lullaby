local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Apply Gamma to gl_FragColor',
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      code: |||
        #include "lullaby/data/shaders/pbr/gamma.glslh"
      |||,
      main_code: |||
        gl_FragColor.rgb = ApplyGamma(gl_FragColor.rgb);
      |||,
    },
    {
      name: 'Apply Gamma to gl_FragColor',
      outputs: [{
        name: 'outColor',
        type: 'Vec4f',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 300,
        max_version: 0,
      }],
      code: |||
        #include "lullaby/data/shaders/pbr/gamma.glslh"
      |||,
      main_code: |||
        outColor.rgb = ApplyGamma(outColor.rgb);
      |||,
    },
  ],
}
