local utils = import 'third_party/lullaby/data/jsonnet/utils.jsonnet';
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
      code: |||
        #include "third_party/lullaby/data/lullshaders/include/premultiply_alpha.glslh"
      |||,
      main_code: |||
        gl_FragColor *= PremultiplyAlpha(vColor);
      |||,
    },
  ],
}
