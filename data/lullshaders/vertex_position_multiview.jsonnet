local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Multiview Transform',
      features: [utils.hash('Transform')],
      environment: [utils.hash('MULTIVIEW')],
      inputs: [{
        name: 'aPosition',
        type: 'Vec4f',
        usage: 'Position',
      }],
      uniforms: [{
        name: 'model_view_projection',
        type: 'Float4x4',
        array_size: 2,
      }],
      code: |||
        #extension GL_OVR_multiview2 : require
        layout(num_views = 2) in;
      |||,
      main_code: |||
        gl_Position = UNIFORM(model_view_projection)[gl_ViewID_OVR] * aPosition;
      |||,
    },
  ],
}
