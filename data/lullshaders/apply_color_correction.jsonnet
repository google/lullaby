local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Apply color correction to gl_FragColor',
      uniforms: [{
        name: 'color_correction',
        type: 'Float4',
        values: [1.0, 1.0, 1.0, 1.0],
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      code: |||
        const float kMiddleGrayGamma = 0.466;
      |||,
      main_code: |||
        vec3 color = gl_FragColor.rgb;
        vec3 colorShift = UNIFORM(color_correction).rgb;
        float averagePixelIntensity = UNIFORM(color_correction).a;
        color *= colorShift * (averagePixelIntensity/kMiddleGrayGamma);
        gl_FragColor.rgb = color;
      |||,
    },
    {
      name: 'Apply color correction to outColor',
      uniforms: [{
        name: 'color_correction',
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
        const float kMiddleGrayGamma = 0.466;
      |||,
      main_code: |||
        vec3 color = outColor.rgb;
        vec3 colorShift = UNIFORM(color_correction).rgb;
        float averagePixelIntensity = UNIFORM(color_correction).a;
        color *= colorShift * (averagePixelIntensity/kMiddleGrayGamma);
        outColor.rgb = color;
      |||,
    },
  ],
}
