local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Normal (Vertex Attribute)',
      inputs: [{
        name: 'vNormal',
        type: 'Vec3f',
      }],
      code: |||
        vec3 normal;
      |||,
      main_code: |||
        normal = normalize(vNormal);
      |||,
    },
    {
      name: 'Normal (Texture)',
      environment: [utils.hash('Texture_Normal')],
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }, {
        name: 'vTangent',
        type: 'Vec3f',
      }, {
        name: 'vBitangent',
        type: 'Vec3f',
      }, {
        name: 'vNormal',
        type: 'Vec3f',
      }],
      samplers: [{
        name: 'NormalTexture',
        usage: 'Normal',
        type: 'Standard2d',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      main_code: |||
        vec3 raw_normal = texture2D(SAMPLER(NormalTexture), vTexCoord).xyz;
        vec3 texture_space_normal = 2. * raw_normal - 1.;
        mat3 orientation_matrix = mat3(normalize(vTangent), normalize(vBitangent), normalize(vNormal));
        normal = orientation_matrix * texture_space_normal;
      |||,
    },
    {
      name: 'Normal (Texture)',
      environment: [utils.hash('Texture_Normal')],
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }, {
        name: 'vTangent',
        type: 'Vec3f',
      }, {
        name: 'vBitangent',
        type: 'Vec3f',
      }, {
        name: 'vNormal',
        type: 'Vec3f',
      }],
      samplers: [{
        name: 'NormalTexture',
        usage: 'Normal',
        type: 'Standard2d',
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 300,
        max_version: 0,
      }],
      main_code: |||
        vec3 raw_normal = texture(SAMPLER(NormalTexture), vTexCoord).xyz;
        vec3 texture_space_normal = 2. * raw_normal - 1.;
        mat3 orientation_matrix = mat3(normalize(vTangent), normalize(vBitangent), normalize(vNormal));
        normal = orientation_matrix * texture_space_normal;
      |||,
    },
  ],
}
