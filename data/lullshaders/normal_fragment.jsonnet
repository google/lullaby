local utils = import 'third_party/lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: "Normal (Vertex Attribute)",
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
      name: "Normal (Texture)",
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
      main_code: |||
        vec3 raw_normal = texture2D(NormalTexture, vTexCoord).xyz;
        vec3 texture_space_normal = 2. * raw_normal - 1.;
        mat3 orientation_matrix = mat3(vTangent, vBitangent, vNormal);
        normal = normalize(orientation_matrix * texture_space_normal);
      |||,
    },
  ],
}
