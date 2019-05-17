local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Base Color (White)',
      code: |||
        vec4 base_color;
      |||,
      main_code: |||
        base_color = vec4(1);
      |||,
    },
    {
      name: 'Base Color (Uniform)',
      uniforms: [{
        name: 'BaseColor',
        type: 'Float4',
        values: [1.0, 1.0, 1.0, 1.0],
      }],
      main_code: |||
        base_color *= UNIFORM(BaseColor);
      |||,
    },
    {
      name: 'Base Color (Vertex)',
      inputs: [{
        name: 'vColor',
        type: 'Vec4f',
      }],
      main_code: |||
        base_color *= vColor;
      |||,
    },
    {
      name: 'Base Color (Texture)',
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }],
      samplers: [{
        name: 'BaseColorTexture',
        usage: 'BaseColor',
        type: 'Standard2d',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      code: |||
        // TODO move this into a util folder:
        #include "lullaby/data/shaders/pbr/gamma.glslh"
      |||,
      main_code: |||
        // TODO Get rid of the RemoveGamma call.
        base_color *=
            RemoveGamma(texture2D(SAMPLER(BaseColorTexture), vTexCoord));
      |||,
    },
    {
      name: 'Base Color (Texture)',
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }],
      samplers: [{
        name: 'BaseColorTexture',
        usage: 'BaseColor',
        type: 'Standard2d',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 300,
        max_version: 0,
      }],
      code: |||
        // TODO move this into a util folder:
        #include "lullaby/data/shaders/pbr/gamma.glslh"
      |||,
      main_code: |||
        // TODO Get rid of the RemoveGamma call.
        base_color *=
            RemoveGamma(texture(SAMPLER(BaseColorTexture), vTexCoord));
      |||,
    },
    {
      name: 'Base Color (multiply by color)',
      uniforms: [{
        name: 'color',
        type: 'Float4',
        values: [1.0, 1.0, 1.0, 1.0],
      }],
      main_code: |||
        base_color.rgb *= UNIFORM(color).rgb;
      |||,
    },
  ],
}
