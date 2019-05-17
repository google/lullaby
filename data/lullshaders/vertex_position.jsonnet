local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
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
      outputs: [{
        name: 'vViewDirection',
        type: 'Vec3f',
      }],
      uniforms: [{
        name: 'model_view_projection',
        type: 'Float4x4',
      }, {
        name: 'model',
        type: 'Float4x4',
      }, {
        name: 'camera_pos',
        type: 'Float3',
      }],
      code: |||
        // This will be consumed by the view_direction snippet.
        vec3 world_position;
      |||,
      main_code: |||
        gl_Position = UNIFORM(model_view_projection) * aPosition;
        world_position = (UNIFORM(model) * aPosition).xyz;
      |||,
    },
    {
      name: 'World Position Output',
      inputs: [{
        name: 'aPosition',
        type: 'Vec4f',
        usage: 'Position',
      }],
      outputs: [{
        name: 'vVertPos',
        type: 'Vec3f',
      }],
      main_code: |||
        vVertPos = world_position;
      |||,
    },
  ],
}
