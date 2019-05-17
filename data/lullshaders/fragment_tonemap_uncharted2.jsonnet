local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
// This tonemap is based on the Uncharted 2 Tone Mapper detailed here:
// http://filmicworlds.com/blog/filmic-tonemapping-operators/
{
  snippets: [
    {
      name: 'Uncharted2 Tonemapping',
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      uniforms: [{
        name: 'exposure',
        type: 'Float1',
        values: [1.0],
      }],
      code: |||
        const float A = 0.15;
        const float B = 0.50;
        const float C = 0.10;
        const float D = 0.20;
        const float E = 0.02;
        const float F = 0.30;
        const float W = 11.2;
        vec3 Uncharted2Tonemap(vec3 x) {
          return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
        }
      |||,
      main_code: |||
        vec3 tonemapped = Uncharted2Tonemap(gl_FragColor.rgb);
        vec3 white_scale = 1.0/Uncharted2Tonemap(vec3(W));
        gl_FragColor.rgb = tonemapped * white_scale;
      |||,
    },
    {
      name: 'Uncharted2 Tonemapping',
      outputs: [{
        name: 'outColor',
        type: 'Vec4f',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 300,
        max_version: 0,
      }],
      uniforms: [{
        name: 'exposure',
        type: 'Float1',
        values: [1.0],
      }],
      code: |||
        const float A = 0.15;
        const float B = 0.50;
        const float C = 0.10;
        const float D = 0.20;
        const float E = 0.02;
        const float F = 0.30;
        const float W = 11.2;
        vec3 Uncharted2Tonemap(vec3 x) {
          return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
        }
      |||,
      main_code: |||
        vec3 tonemapped = Uncharted2Tonemap(outColor.rgb);
        vec3 white_scale = 1.0/Uncharted2Tonemap(vec3(W));
        outColor.rgb = tonemapped * white_scale;
      |||,
    },
  ],
}
