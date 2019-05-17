local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Occlusion Roughness Metallic (Globals)',
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
      name: 'Roughness (Uniform)',
      uniforms: [{
        name: 'Roughness',
        type: 'Float1',
        values: [1.0],
      }],
      main_code: |||
        roughness *= UNIFORM(Roughness);
      |||,
    },
    {
      name: 'Metallic (Uniform)',
      uniforms: [{
        name: 'Metallic',
        type: 'Float1',
        values: [1.0],
      }],
      main_code: |||
        metallic *= UNIFORM(Metallic);
      |||,
    },
    {
      name: 'Occlusion Roughness Metallic (Texture red/green/blue channels)',
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }],
      samplers: [{
        name: 'OcclusionRoughnessMetallicTexture',
        usage_per_channel: ['Occlusion', 'Roughness', 'Metallic'],
        type: 'Standard2d',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      main_code: |||
        vec3 orm = texture2D(
            SAMPLER(OcclusionRoughnessMetallicTexture), vTexCoord).xyz;
        occlusion *= orm.x;
        roughness *= orm.y;
        metallic *= orm.z;
      |||,
    },
    {
      name: 'Occlusion Roughness Metallic (Texture red/green/blue channels)',
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }],
      samplers: [{
        name: 'OcclusionRoughnessMetallicTexture',
        usage_per_channel: ['Occlusion', 'Roughness', 'Metallic'],
        type: 'Standard2d',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 300,
        max_version: 0,
      }],
      main_code: |||
        vec3 orm = texture(
            SAMPLER(OcclusionRoughnessMetallicTexture), vTexCoord).xyz;
        occlusion *= orm.x;
        roughness *= orm.y;
        metallic *= orm.z;
      |||,
    },
    {
      name: 'Occlusion (Texture red channel alt UVs)',
      features: [utils.hash('Occlusion')],
      inputs: [{
        name: 'vTexCoordAlt',
        type: 'Vec2f',
      }],
      samplers: [{
        name: 'OcclusionTexture',
        usage_per_channel: ['Occlusion'],
        type: 'Standard2d',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 300,
        max_version: 0,
      }],
      main_code: |||
        occlusion *= texture(SAMPLER(OcclusionTexture), vTexCoordAlt).x;
      |||,
    },
    {
      name: 'Occlusion (Texture red channel)',
      features: [utils.hash('Occlusion')],
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }],
      samplers: [{
        name: 'OcclusionTexture',
        usage_per_channel: ['Occlusion'],
        type: 'Standard2d',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 300,
        max_version: 0,
      }],
      main_code: |||
        occlusion *= texture(SAMPLER(OcclusionTexture), vTexCoord).x;
      |||,
    },
    {
      name: 'Roughness Metallic (Texture, green/blue channels)',
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }],
      samplers: [{
        name: 'RoughnessMetallicTexture',
        usage_per_channel: ['Unused', 'Roughness', 'Metallic'],
        type: 'Standard2d',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      main_code: |||
        vec2 rm = texture2D(SAMPLER(RoughnessMetallicTexture), vTexCoord).yz;
        roughness *= rm.x;
        metallic *= rm.y;
      |||,
    },
    {
      name: 'Roughness Metallic (Texture, green/blue channels)',
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }],
      samplers: [{
        name: 'RoughnessMetallicTexture',
        usage_per_channel: ['Unused', 'Roughness', 'Metallic'],
        type: 'Standard2d',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 300,
        max_version: 0,
      }],
      main_code: |||
        vec2 rm = texture(SAMPLER(RoughnessMetallicTexture), vTexCoord).yz;
        roughness *= rm.x;
        metallic *= rm.y;
      |||,
    },
    {
      name: 'Roughness (Texture, red channel)',
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }],
      samplers: [{
        name: 'RoughnessTexture',
        usage_per_channel: ['Roughness'],
        type: 'Standard2d',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      main_code: |||
        roughness *= texture2D(SAMPLER(RoughnessTexture), vTexCoord).x;
      |||,
    },
    {
      name: 'Roughness (Texture, red channel)',
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }],
      samplers: [{
        name: 'RoughnessTexture',
        usage_per_channel: ['Roughness'],
        type: 'Standard2d',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 300,
        max_version: 0,
      }],
      main_code: |||
        roughness *= texture(SAMPLER(RoughnessTexture), vTexCoord).x;
      |||,
    },
    {
      name: 'Metallic (Texture, red channel)',
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }],
      samplers: [{
        name: 'MetallicTexture',
        usage_per_channel: ['Metallic'],
        type: 'Standard2d',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      main_code: |||
        metallic *= texture2D(SAMPLER(MetallicTexture), vTexCoord).x;
      |||,
    },
    {
      name: 'Metallic (Texture, red channel)',
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }],
      samplers: [{
        name: 'MetallicTexture',
        usage_per_channel: ['Metallic'],
        type: 'Standard2d',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 300,
        max_version: 0,
      }],
      main_code: |||
        metallic *= texture(SAMPLER(MetallicTexture), vTexCoord).x;
      |||,
    },
  ],
}
