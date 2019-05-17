local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
  {
      name: 'Indirect Light (Environment Map BRDF Texture)',
      features: [utils.hash('IBL_RGBM')],
      samplers: [
        {
          name: 'DiffuseEnvTexture',
          usage: 'DiffuseEnvironment',
          type: 'CubeMap',
        },
        {
          name: 'SpecularEnvTexture',
          usage: 'SpecularEnvironment',
          type: 'CubeMap',
        },
        {
          name: 'BrdfLookUpTableTexture',
          usage: 'BrdfLookupTable',
          type: 'Standard2d',
        },
      ],
      uniforms: [{
        name: 'light_environment_color_factor',
        type: 'Float3',
        values: [1.0, 1.0, 1.0],
      }, {
        name: 'num_mips',
        type: 'Float1',
        values: [0.0],
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      code: |||
        #include "lullaby/data/shaders/pbr/rgbm_to_rgb.glslh"
        vec3 GetIndirectLight(vec3 normal, vec3 reflection, float n_dot_v,
                              vec3 in_diffuse_color, vec3 in_specular_color) {
          vec3 brdf = texture2D(SAMPLER(BrdfLookUpTableTexture),
              vec2(n_dot_v, 1.0 - roughness)).rgb;
          vec3 diffuse_light =
              textureCube(SAMPLER(DiffuseEnvTexture), normal).rgb;

          float lod = roughness * UNIFORM(num_mips);
          vec3 specular_light =
              RgbmToRgbSpecular(
              textureCubeLod(SAMPLER(SpecularEnvTexture), reflection, lod));

          vec3 diffuse = diffuse_light * in_diffuse_color;
          vec3 specular = specular_light *
              (in_specular_color * brdf.x + brdf.y);

          return (diffuse + specular) * UNIFORM(light_environment_color_factor);
        }
      |||,
    },
    {
      name: 'Indirect Light (Environment Map BRDF Texture)',
      features: [utils.hash('IBL_RGBM')],
      samplers: [
        {
          name: 'DiffuseEnvTexture',
          usage: 'DiffuseEnvironment',
          type: 'CubeMap',
        },
        {
          name: 'SpecularEnvTexture',
          usage: 'SpecularEnvironment',
          type: 'CubeMap',
        },
        {
          name: 'BrdfLookUpTableTexture',
          usage: 'BrdfLookupTable',
          type: 'Standard2d',
        },
      ],
      uniforms: [{
        name: 'light_environment_color_factor',
        type: 'Float3',
        values: [1.0, 1.0, 1.0],
      }, {
        name: 'num_mips',
        type: 'Float1',
        values: [0.0],
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
        #include "lullaby/data/shaders/pbr/rgbm_to_rgb.glslh"
        vec3 GetIndirectLight(vec3 normal, vec3 reflection, float n_dot_v,
                              vec3 in_diffuse_color, vec3 in_specular_color) {
          vec3 brdf = texture(SAMPLER(BrdfLookUpTableTexture),
              vec2(n_dot_v, 1.0 - roughness)).rgb;
          vec3 diffuse_light = texture(SAMPLER(DiffuseEnvTexture), normal).rgb;

          float lod = roughness * UNIFORM(num_mips);
          vec3 specular_light =
              RgbmToRgbSpecular(
              textureLod(SAMPLER(SpecularEnvTexture), reflection, lod));

          vec3 diffuse = diffuse_light * in_diffuse_color;
          vec3 specular = specular_light *
              (in_specular_color * brdf.x + brdf.y);

          return (diffuse + specular) * UNIFORM(light_environment_color_factor);
        }
      |||,
    },
    {
      name: 'Indirect Light (Environment Map BRDF Computed)',
      features: [utils.hash('IBL_RGBM')],
      samplers: [
        {
          name: 'DiffuseEnvTexture',
          usage: 'DiffuseEnvironment',
          type: 'CubeMap',
        },
        {
          name: 'SpecularEnvTexture',
          usage: 'SpecularEnvironment',
          type: 'CubeMap',
        },
      ],
      uniforms: [{
        name: 'light_environment_color_factor',
        type: 'Float3',
        values: [1.0, 1.0, 1.0],
      }, {
        name: 'num_mips',
        type: 'Float1',
        values: [0.0],
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      code: |||
        #include "lullaby/data/shaders/pbr/rgbm_to_rgb.glslh"
        #include "lullaby/data/shaders/pbr/pbr_surface_common.glslh"

        vec3 GetIndirectLight(vec3 normal, vec3 reflection, float n_dot_v,
                              vec3 in_diffuse_color, vec3 in_specular_color) {
          vec3 diffuse_light = textureCube(DiffuseEnvTexture, normal).rgb;

          float lod = roughness * UNIFORM(num_mips);
          vec3 specular_light =
              RgbmToRgbSpecular(
              textureCubeLod(SAMPLER(SpecularEnvTexture), reflection, lod));

          vec3 diffuse = diffuse_light * in_diffuse_color;
          vec3 specular = specular_light *
              EnvBRDFApproxKaris(roughness, n_dot_v, in_specular_color);

          return (diffuse + specular) * UNIFORM(light_environment_color_factor);
        }
      |||,
    },
    {
      name: 'Indirect Light (Environment Map BRDF Computed)',
      features: [utils.hash('IBL_RGBM')],
      samplers: [
        {
          name: 'DiffuseEnvTexture',
          usage: 'DiffuseEnvironment',
          type: 'CubeMap',
        },
        {
          name: 'SpecularEnvTexture',
          usage: 'SpecularEnvironment',
          type: 'CubeMap',
        },
      ],
      uniforms: [{
        name: 'light_environment_color_factor',
        type: 'Float3',
        values: [1.0, 1.0, 1.0],
      }, {
        name: 'num_mips',
        type: 'Float1',
        values: [0.0],
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
        #include "lullaby/data/shaders/pbr/rgbm_to_rgb.glslh"
        #include "lullaby/data/shaders/pbr/pbr_surface_common.glslh"

        vec3 GetIndirectLight(vec3 normal, vec3 reflection, float n_dot_v,
                              vec3 in_diffuse_color, vec3 in_specular_color) {
          vec3 diffuse_light = texture(SAMPLER(DiffuseEnvTexture), normal).rgb;

          float lod = roughness * UNIFORM(num_mips);
          vec3 specular_light =
              RgbmToRgbSpecular(
              textureLod(SAMPLER(SpecularEnvTexture), reflection, lod));

          vec3 diffuse = diffuse_light * in_diffuse_color;
          vec3 specular = specular_light *
              EnvBRDFApproxKaris(roughness, n_dot_v, in_specular_color);

          return (diffuse + specular) * UNIFORM(light_environment_color_factor);
        }
      |||,
    },
    {
      name: 'Indirect Light (Environment Map BRDF Texture)',
      features: [utils.hash('IBL')],
      samplers: [
        {
          name: 'DiffuseEnvTexture',
          usage: 'DiffuseEnvironment',
          type: 'CubeMap',
        },
        {
          name: 'SpecularEnvTexture',
          usage: 'SpecularEnvironment',
          type: 'CubeMap',
        },
        {
          name: 'BrdfLookUpTableTexture',
          usage: 'BrdfLookupTable',
          type: 'Standard2d',
        },
      ],
      uniforms: [{
        name: 'light_environment_color_factor',
        type: 'Float3',
        values: [1.0, 1.0, 1.0],
      }, {
        name: 'num_mips',
        type: 'Float1',
        values: [0.0],
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      code: |||
        vec3 GetIndirectLight(vec3 normal, vec3 reflection, float n_dot_v,
                              vec3 in_diffuse_color, vec3 in_specular_color) {
          vec3 brdf = texture2D(SAMPLER(BrdfLookUpTableTexture),
              vec2(n_dot_v, 1.0 - roughness)).rgb;
          vec3 diffuse_light =
              textureCube(SAMPLER(DiffuseEnvTexture), normal).rgb;

          float lod = roughness * UNIFORM(num_mips);
          vec3 specular_light =
              textureCubeLod(SAMPLER(SpecularEnvTexture), reflection, lod).rgb;

          vec3 diffuse = diffuse_light * in_diffuse_color;
          vec3 specular = specular_light *
              (in_specular_color * brdf.x + brdf.y);

          return (diffuse + specular) * UNIFORM(light_environment_color_factor);
        }
      |||,
    },
    {
      name: 'Indirect Light (Environment Map BRDF Texture)',
      features: [utils.hash('IBL')],
      samplers: [
        {
          name: 'DiffuseEnvTexture',
          usage: 'DiffuseEnvironment',
          type: 'CubeMap',
        },
        {
          name: 'SpecularEnvTexture',
          usage: 'SpecularEnvironment',
          type: 'CubeMap',
        },
        {
          name: 'BrdfLookUpTableTexture',
          usage: 'BrdfLookupTable',
          type: 'Standard2d',
        },
      ],
      uniforms: [{
        name: 'light_environment_color_factor',
        type: 'Float3',
        values: [1.0, 1.0, 1.0],
      }, {
        name: 'num_mips',
        type: 'Float1',
        values: [0.0],
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
        vec3 GetIndirectLight(vec3 normal, vec3 reflection, float n_dot_v,
                              vec3 in_diffuse_color, vec3 in_specular_color) {
          vec3 brdf = texture(SAMPLER(BrdfLookUpTableTexture),
              vec2(n_dot_v, 1.0 - roughness)).rgb;
          vec3 diffuse_light = texture(SAMPLER(DiffuseEnvTexture), normal).rgb;

          float lod = roughness * UNIFORM(num_mips);
          vec3 specular_light =
              textureLod(SAMPLER(SpecularEnvTexture), reflection, lod).rgb;

          vec3 diffuse = diffuse_light * in_diffuse_color;
          vec3 specular = specular_light *
              (in_specular_color * brdf.x + brdf.y);

          return (diffuse + specular) * UNIFORM(light_environment_color_factor);
        }
      |||,
    },
    {
      name: 'Indirect Light (Environment Map BRDF Computed)',
      features: [utils.hash('IBL')],
      samplers: [
        {
          name: 'DiffuseEnvTexture',
          usage: 'DiffuseEnvironment',
          type: 'CubeMap',
        },
        {
          name: 'SpecularEnvTexture',
          usage: 'SpecularEnvironment',
          type: 'CubeMap',
        },
      ],
      uniforms: [{
        name: 'light_environment_color_factor',
        type: 'Float3',
        values: [1.0, 1.0, 1.0],
      }, {
        name: 'num_mips',
        type: 'Float1',
        values: [0.0],
      }],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
      code: |||
        #include "lullaby/data/shaders/pbr/pbr_surface_common.glslh"

        vec3 GetIndirectLight(vec3 normal, vec3 reflection, float n_dot_v,
                              vec3 in_diffuse_color, vec3 in_specular_color) {
          vec3 diffuse_light =
              textureCube(SAMPLER(DiffuseEnvTexture), normal).rgb;

          float lod = roughness * UNIFORM(num_mips);
          vec3 specular_light =
              textureCubeLod(SAMPLER(SpecularEnvTexture), reflection, lod).rgb;

          vec3 diffuse = diffuse_light * in_diffuse_color;
          vec3 specular = specular_light *
              EnvBRDFApproxKaris(roughness, n_dot_v, in_specular_color);

          return (diffuse + specular) * UNIFORM(light_environment_color_factor);
        }
      |||,
    },
    {
      name: 'Indirect Light (Environment Map BRDF Computed)',
      features: [utils.hash('IBL')],
      samplers: [
        {
          name: 'DiffuseEnvTexture',
          usage: 'DiffuseEnvironment',
          type: 'CubeMap',
        },
        {
          name: 'SpecularEnvTexture',
          usage: 'SpecularEnvironment',
          type: 'CubeMap',
        },
      ],
      uniforms: [{
        name: 'light_environment_color_factor',
        type: 'Float3',
        values: [1.0, 1.0, 1.0],
      }, {
        name: 'num_mips',
        type: 'Float1',
        values: [0.0],
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
        #include "lullaby/data/shaders/pbr/pbr_surface_common.glslh"

        vec3 GetIndirectLight(vec3 normal, vec3 reflection, float n_dot_v,
                              vec3 in_diffuse_color, vec3 in_specular_color) {
          vec3 diffuse_light = texture(SAMPLER(DiffuseEnvTexture), normal).rgb;

          float lod = roughness * UNIFORM(num_mips);
          vec3 specular_light =
              textureLod(SAMPLER(SpecularEnvTexture), reflection, lod).rgb;

          vec3 diffuse = diffuse_light * in_diffuse_color;
          vec3 specular = specular_light *
              EnvBRDFApproxKaris(roughness, n_dot_v, in_specular_color);

          return (diffuse + specular) * UNIFORM(light_environment_color_factor);
        }
      |||,
    },
    {
      name: 'Omnidirectional (Ambient) Indirect Light',
      features: [utils.hash('IBL')],
      uniforms: [{
        name: 'light_ambient_color',
        type: 'Float3',
        array_size: 1,
      }],
      code: |||
        #include "lullaby/data/lullshaders/include/phong.glslh"
        #include "lullaby/data/shaders/pbr/pbr_surface_common.glslh"

        vec3 GetIndirectLight(vec3 normal, vec3 reflection, float n_dot_v,
                              vec3 diffuse_color, vec3 specular_color) {
          vec3 ambient = vec3(0);
          LIGHT_CALCULATE_AMBIENTS(ambient, 1);
          return ambient * (diffuse_color + EnvBRDFApproxKaris(roughness,
                   n_dot_v, specular_color));
        }
      |||
    },
    {
      name: 'HardcodedIndirectLight',
      features: [utils.hash('IBL')],
      code: |||
        #include "lullaby/data/shaders/pbr/pbr_surface_common.glslh"

        vec3 EnvironmentSphericalHarmonics(const vec3 direction) {
          // Hardcoded spherical harmonics that look generally gray-blue, with
          // more blue up, orange down
          return max(
              vec3(.75, .75, .8)
            + direction.y * vec3(-.1, .1, .3)
            + direction.z * vec3(0)
            + direction.x * vec3(0)
            , 0.0);
        }

        vec3 SampleEnvironment(vec3 direction) {
          // TODO:  Get Environment maps working.
          return mix(vec3(.2), vec3(.2, .3, .6),
                     pow(clamp(direction.y, 0., 1.), .3));
        }

        vec3 GetIndirectLight(vec3 normal, vec3 reflection, float n_dot_v,
                              vec3 diffuse_color, vec3 specular_color) {
          vec3 color;

          // Indirect diffuse
          color = diffuse_color * EnvironmentSphericalHarmonics(normal) *
                  Lambert();
          // Indirect specular
          color += SampleEnvironment(reflection) * EnvBRDFApproxKaris(roughness,
                   n_dot_v, specular_color);

          return color;
        }
      |||,
    },
  ],
}
