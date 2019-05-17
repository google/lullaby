local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Uniform Color',
      features: [utils.hash('UniformColor')],
      uniforms: [{
        name: 'color',
        type: 'Float4',
        values: [1.0, 1.0, 1.0, 1.0],
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      code: |||
        #include "lullaby/data/lullshaders/include/premultiply_alpha.glslh"
      |||,
      main_code: |||
        gl_FragColor *= PremultiplyAlpha(UNIFORM(color));
      |||,
    },
    {
      name: 'Uniform Color',
      features: [utils.hash('UniformColor')],
      uniforms: [{
        name: 'color',
        type: 'Float4',
        values: [1.0, 1.0, 1.0, 1.0],
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
      code: |||
        #include "lullaby/data/lullshaders/include/premultiply_alpha.glslh"
      |||,
      main_code: |||
        outColor *= PremultiplyAlpha(UNIFORM(color));
      |||,
    },
  ],
}
