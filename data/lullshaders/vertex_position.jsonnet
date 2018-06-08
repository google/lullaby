local utils = import 'third_party/lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Transform',
      features: [utils.hash('Transform')],
      inputs: [{
        name: 'aPosition',
        type: 'Vec4f',
        usage: 'Position',
      }],
      uniforms: [{
        name: 'model_view_projection',
        type: 'Float4x4',
      }],
      main_code: |||
        gl_Position = model_view_projection * aPosition;
      |||,
    },
  ],
}
