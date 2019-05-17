local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Light Integration',
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
        accumulated_light += GetIndirectLight(normal, reflection, n_dot_v,
                                  diffuse_color, specular_color) * occlusion;

        gl_FragColor =
            PremultiplyAlpha(vec4(accumulated_light, base_color.w * UNIFORM(color).a));
      |||,
    },
    {
      name: 'Light Integration',
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
        accumulated_light += GetIndirectLight(normal, reflection, n_dot_v,
                                 diffuse_color, specular_color) * occlusion;

        outColor = PremultiplyAlpha(
            vec4(accumulated_light, base_color.w * UNIFORM(color).a));
      |||,
    },
  ],
}
