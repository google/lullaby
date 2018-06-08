local utils = import 'third_party/lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: "Base Color (White)",
      code: |||
        vec4 base_color;
      |||,
      main_code: |||
        base_color = vec4(1);
      |||,
    },
    {
      name: "Base Color (Uniform)",
      uniforms: [{
        name: 'BaseColor',
        type: 'Float4',
        values: [1.0, 1.0, 1.0, 1.0],
      }],
      main_code: |||
        base_color *= BaseColor;
      |||,
    },
    {
      name: "Base Color (Texture)",
      environment: [utils.hash('Texture_BaseColor')],
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }],
      samplers: [{
        name: 'BaseColorTexture',
        usage: 'BaseColor',
        type: 'Standard2d',
      }],
      code: |||
        // TODO(b/79531567) move this into a util folder:
        #include "third_party/lullaby/data/shaders/pbr/gamma.glslh"
      |||,
      main_code: |||
        // TODO(b/79588609) Get rid of the RemoveGamma call.
        base_color *= RemoveGamma(texture2D(BaseColorTexture, vTexCoord));
      |||,
    },
  ],
}
