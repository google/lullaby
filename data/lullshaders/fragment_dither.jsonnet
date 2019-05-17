local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Uniform Alpha',
      features: [utils.hash('DitherAlpha')],
      uniforms: [{
        name: 'dither_alpha',
        type: 'Float1',
        values: [1.0],
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 100,
      }],
      code: |||
        lowp float Dither3x3() {
          ivec2 screen_pixel = ivec2(mod(gl_FragCoord.xy, vec2(3)));
          return mat3(
              0.5 / 9.0, 7.5 / 9.0, 3.5 / 9.0,
              6.5 / 9.0, 5.5 / 9.0, 2.5 / 9.0,
              4.5 / 9.0, 1.5 / 9.0, 8.5 / 9.0
            )[screen_pixel.x][screen_pixel.y];
        }
      |||,
      main_code: |||
        // Dither based transparency.
        if (UNIFORM(dither_alpha) < Dither3x3()) {
          discard;
        }
      |||,
    },
    {
      name: 'Uniform Alpha',
      features: [utils.hash('DitherAlpha')],
      uniforms: [{
        name: 'dither_alpha',
        type: 'Float1',
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
        lowp float Dither3x3() {
          ivec2 screen_pixel = ivec2(mod(gl_FragCoord.xy, vec2(3)));
          return mat3(
              0.5 / 9.0, 7.5 / 9.0, 3.5 / 9.0,
              6.5 / 9.0, 5.5 / 9.0, 2.5 / 9.0,
              4.5 / 9.0, 1.5 / 9.0, 8.5 / 9.0
            )[screen_pixel.x][screen_pixel.y];
        }
      |||,
      main_code: |||
        // Dither based transparency.
        if (UNIFORM(dither_alpha) < Dither3x3()) {
          discard;
        }
      |||,
    },
  ],
}
