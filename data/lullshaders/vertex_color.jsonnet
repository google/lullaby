local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Vertex Color',
      features: [utils.hash('VertexColor')],
      inputs: [{
        name: 'aColor',
        type: 'Vec4f',
        usage: 'Color',
      }],
      outputs: [{
        name: 'vColor',
        type: 'Vec4f',
      }],
      main_code: |||
        vColor = aColor;
      |||,
    },
  ],
}
