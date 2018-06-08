local utils = import 'third_party/lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: "Occlusion Roughness Metallic (Globals)",
      code: |||
        float occlusion;
        float roughness;
        float metallic;
      |||,
      main_code: |||
        occlusion = 1.;
        roughness = 1.;
        metallic = 1.;
      |||,
    },
    {
      name: "Roughness (Uniform)",
      uniforms: [{
        name: 'Roughness',
        type: 'Float1',
        values: [1.0],
      }],
      main_code: |||
        roughness *= Roughness;
      |||,
    },
    {
      name: "Metallic (Uniform)",
      uniforms: [{
        name: 'Metallic',
        type: 'Float1',
        values: [1.0],
      }],
      main_code: |||
        metallic *= Metallic;
      |||,
    },
    {
      name: "Occlusion Roughness Metallic (Texture)",
      features: [
        utils.hash('Texture_Occlusion'),
        utils.hash('Texture_Roughness'),
        utils.hash('Texture_Metallic'),
      ],
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }],
      samplers: [{
        name: 'OcclusionRoughnessMetallicTexture',
        usage: 'Roughness',
        type: 'Standard2d',
      }],
      main_code: |||
        vec3 orm = texture2D(OcclusionRoughnessMetallicTexture, vTexCoord).xyz;
        occlusion *= orm.x;
        roughness *= orm.y;
        metallic *= orm.z;
      |||,
    },
    {
      name: "Roughness Metallic (Texture)",
      features: [
        utils.hash('Texture_Roughness'),
        utils.hash('Texture_Metallic'),
      ],
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }],
      samplers: [{
        name: 'RoughnessMetallicTexture',
        usage: 'Roughness',
        type: 'Standard2d',
      }],
      main_code: |||
        vec2 rm = texture2D(RoughnessMetallicTexture, vTexCoord).yz;
        roughness *= rm.x;
        metallic *= rm.y;
      |||,
    },
  ],
}
