local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Phong',
      features: [utils.hash('Light')],
      inputs: [{
        name: 'vVertPos',
        type: 'Vec3f',
      }, {
        name: 'vNormal',
        type: 'Vec3f',
      }],
      uniforms: [{
        name: 'camera_dir',
        type: 'Float3',
      }, {
        name: 'light_ambient_color',
        type: 'Float3',
        array_size: 1,
        values: [1.0, 1.0, 1.0],
      }, {
        name: 'light_directional_dir',
        type: 'Float3',
        array_size: 1,
        values: [0.0, -1.0, 0.0],
      }, {
        name: 'light_directional_color',
        type: 'Float3',
        array_size: 1,
        values: [0.0, 0.0, 0.0],
      }, {
        name: 'light_directional_exponent',
        type: 'Float1',
        array_size: 1,
        values: [1.0],
      }, {
        name: 'light_point_pos',
        type: 'Float3',
        array_size: 1,
        values: [1.0, 5.0, 5.0],
      }, {
        name: 'light_point_color',
        type: 'Float3',
        array_size: 1,
        values: [0.0, 0.0, 0.0],
      }, {
        name: 'light_point_exponent',
        type: 'Float1',
        array_size: 1,
        values: [1.0],
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      code: |||
        #include "lullaby/data/lullshaders/include/phong.glslh"
      |||,
      main_code: |||
        // Re-normalize the normal which been could have been scaled or
        // interpolated from the vertex stage.
        vec3 normal = normalize(vNormal);

        // Light calculaltions.
        vec3 light_color = vec3(0.0, 0.0, 0.0);
        LIGHT_CALCULATE_AMBIENTS(light_color, 1)
        LIGHT_CALCULATE_DIRECTIONALS(light_color, vVertPos, normal, UNIFORM(camera_dir), 1)
        LIGHT_CALCULATE_POINTS(light_color, vVertPos, normal, UNIFORM(camera_dir), 1)

        gl_FragColor.rgb *= light_color;
      |||,
    },
    {
      name: 'Phong',
      features: [utils.hash('Light')],
      inputs: [{
        name: 'vVertPos',
        type: 'Vec3f',
      }, {
        name: 'vNormal',
        type: 'Vec3f',
      }],
      uniforms: [{
        name: 'camera_dir',
        type: 'Float3',
      }, {
        name: 'light_ambient_color',
        type: 'Float3',
        array_size: 1,
        values: [1.0, 1.0, 1.0],
      }, {
        name: 'light_directional_dir',
        type: 'Float3',
        array_size: 1,
        values: [0.0, -1.0, 0.0],
      }, {
        name: 'light_directional_color',
        type: 'Float3',
        array_size: 1,
        values: [0.0, 0.0, 0.0],
      }, {
        name: 'light_directional_exponent',
        type: 'Float1',
        array_size: 1,
        values: [1.0],
      }, {
        name: 'light_point_pos',
        type: 'Float3',
        array_size: 1,
        values: [1.0, 5.0, 5.0],
      }, {
        name: 'light_point_color',
        type: 'Float3',
        array_size: 1,
        values: [0.0, 0.0, 0.0],
      }, {
        name: 'light_point_exponent',
        type: 'Float1',
        array_size: 1,
        values: [1.0],
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
        #include "lullaby/data/lullshaders/include/phong.glslh"
      |||,
      main_code: |||
        // Re-normalize the normal which been could have been scaled or
        // interpolated from the vertex stage.
        vec3 normal = normalize(vNormal);

        // Light calculaltions.
        vec3 light_color = vec3(0.0, 0.0, 0.0);
        LIGHT_CALCULATE_AMBIENTS(light_color, 1)
        LIGHT_CALCULATE_DIRECTIONALS(light_color, vVertPos, normal, UNIFORM(camera_dir), 1)
        LIGHT_CALCULATE_POINTS(light_color, vVertPos, normal, UNIFORM(camera_dir), 1)

        outColor.rgb *= light_color;
      |||,
    },
  ],
}
