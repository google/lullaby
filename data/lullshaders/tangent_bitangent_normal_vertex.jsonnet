local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: "Tangent Bitangent Normals (Quaternions In)",
      environment: [utils.hash('Texture_Normal')],
      inputs: [{
        name: 'aOrientation',
        type: 'Vec4f',
        usage: 'Orientation',
      }],
      outputs: [{
        name: 'vTangent',
        type: 'Vec3f',
      }, {
        name: 'vBitangent',
        type: 'Vec3f',
      }, {
        name: 'vNormal',
        type: 'Vec3f',
        usage: 'Normal',
      }],
      uniforms: [{
        name: 'mat_normal',
        type: 'Float3x3',
      }],
      code: |||
        mat3 Mat3FromQuaternion(vec4 q) {
          mat3 dest;
          vec3 two_v = 2. * q.xyz;
          vec3 two_v_sq = two_v * q.xyz;
          float two_x_y = two_v.x * q.y;
          float two_x_z = two_v.x * q.z;
          float two_y_z = two_v.y * q.z;
          vec3 two_v_w = two_v * q.w;

          dest[0][0] = 1. - two_v_sq.y - two_v_sq.z;
          dest[1][0] = two_x_y - two_v_w.z;
          dest[2][0] = two_x_z + two_v_w.y;

          dest[0][1] = two_x_y + two_v_w.z;
          dest[1][1] = 1. - two_v_sq.x - two_v_sq.z;
          dest[2][1] = two_y_z - two_v_w.x;

          dest[0][2] = two_x_z - two_v_w.y;
          dest[1][2] = two_y_z + two_v_w.x;
          dest[2][2] = 1. - two_v_sq.x - two_v_sq.y;

          return dest;
        }
      |||,
      main_code: |||
        mat3 tangentBitangentNormal = UNIFORM(mat_normal) *
                                      Mat3FromQuaternion(aOrientation);
        vTangent = tangentBitangentNormal[0];
        vBitangent = tangentBitangentNormal[1];
        vNormal = tangentBitangentNormal[2];
      |||,
    },
    {
      name: "Tangent Bitangent Normals (2 Vectors In)",
      environment: [utils.hash('Texture_Normal')],
      inputs: [{
        name: 'aTangent',
        type: 'Vec4f',
        usage: 'Tangent',
      }, {
        name: 'aNormal',
        type: 'Vec3f',
        usage: 'Normal',
      }],
      outputs: [{
        name: 'vTangent',
        type: 'Vec3f',
      }, {
        name: 'vBitangent',
        type: 'Vec3f',
      }, {
        name: 'vNormal',
        type: 'Vec3f',
        usage: 'Normal',
      }],
      uniforms: [{
        name: 'mat_normal',
        type: 'Float3x3',
      }],
      main_code: |||
        vTangent = UNIFORM(mat_normal) * aTangent.xyz;
        vNormal = UNIFORM(mat_normal) * aNormal;
        vBitangent = cross(vNormal, vTangent) * aTangent.w;
      |||,
    },
  ],
}
