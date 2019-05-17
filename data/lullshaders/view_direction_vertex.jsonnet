local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'View Direction',
      outputs: [{
        name: 'vViewDirection',
        type: 'Vec3f',
      }],
      uniforms: [{
        name: 'camera_pos',
        type: 'Float3',
      }],
      // world_position variable is assumed to exist and be set by a preceding
      // snippet.  We have no way to establish this contract formally at build
      // time.
      main_code: |||
        vViewDirection = UNIFORM(camera_pos) - world_position;
      |||,
    },
  ],
}
