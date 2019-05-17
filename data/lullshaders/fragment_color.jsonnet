local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Vertex Color',
      features: [utils.hash('VertexColor')],
      inputs: [{
        name: 'vColor',
        type: 'Vec4f',
        usage: 'Color',
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
        gl_FragColor *= PremultiplyAlpha(vColor);
      |||,
    },
    {
      name: 'Vertex Color',
      features: [utils.hash('VertexColor')],
      inputs: [{
        name: 'vColor',
        type: 'Vec4f',
        usage: 'Color',
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
        outColor *= PremultiplyAlpha(vColor);
      |||,
    },
  ],
}
