local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Physically Based Rendering',
      inputs: [{
        name: 'vViewDirection',
        type: 'Vec3f',
      }],
      uniforms: [{
        name: 'light_directional_dir',
        type: 'Float3',
        array_size: 1,
        values: [-1.0, -1.0, -1.0],
      }, {
        name: 'light_directional_color',
        type: 'Float3',
        array_size: 1,
        values: [3.14159, 3.14159, 3.14159],
      }],
      code: |||
        #include "lullaby/data/lullshaders/include/premultiply_alpha.glslh"
        #include "lullaby/data/shaders/math.glslh"
        #include "lullaby/data/shaders/pbr/pbr_surface_common.glslh"
      |||,
      main_code: |||
        vec3 light_direction = -normalize(UNIFORM(light_directional_dir)[0]);
        vec3 light_color = UNIFORM(light_directional_color)[0];
        vec3 halfway_vector = normalize(light_direction + view_direction);

        float n_dot_l = max(dot(normal, light_direction), 0.0);
        float n_dot_h = max(dot(normal, halfway_vector), 0.0);
        float v_dot_h = max(dot(view_direction, halfway_vector), 0.0);

        // Calculate the shading terms for the microfacet specular shading model
        vec3 fresnel_term = FresnelLerp(specular_environment_R0,
                                        specular_environment_R90,
                                        v_dot_h);
        float geometric_term = GeometricOcclusionEpic(n_dot_l, n_dot_v,
                                                      alpha_roughness);
        float normal_distribution = DistributionTrowbridgeReitz(alpha_roughness,
                                                                n_dot_h);

        vec3 diffuse_contrib = (1.0 - fresnel_term) *
          LambertianDiffuse(diffuse_color);

        // Apply Torrance-Sparrow BRDF:
        //    D * F * G
        // ---------------
        //  4 * n.L * n.V
        float torrance_sparrow_divisor = 4.0 * n_dot_l * n_dot_v;
        vec3 spec_contrib = fresnel_term * geometric_term *
                            normal_distribution / torrance_sparrow_divisor;
        // If divisor is zero, zero out specular instead of dividing by zero.
        spec_contrib = torrance_sparrow_divisor == 0. ? vec3(0) : spec_contrib;

        // Obtain final intensity as reflectance (BRDF) scaled by the energy of
        // the light (cosine law)
        accumulated_light +=
            n_dot_l * light_color * (diffuse_contrib + spec_contrib);
      |||,
    },
    {
      name: 'Point Light (PBR)',
      features: [utils.hash('PointLight')],
      inputs: [{
        name: 'vViewDirection',
        type: 'Vec3f',
      }, {
        name: 'vVertPos',
        type: 'Vec3f',
      }],
      uniforms: [{
        name: 'light_point_pos',
        type: 'Float3',
        array_size: 4,
        values: [
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
        ],
      }, {
        name: 'light_point_color',
        type: 'Float3',
        array_size: 4,
        values: [
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
          0.0,
        ],
      }],
      code: |||
        #include "lullaby/data/lullshaders/include/premultiply_alpha.glslh"
        #include "lullaby/data/shaders/math.glslh"
        #include "lullaby/data/shaders/pbr/pbr_surface_common.glslh"

        #define MAX_POINT_LIGHTS 4
      |||,
      main_code: |||
        for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
          vec3 light_direction =
              normalize(UNIFORM(light_point_pos)[i] - vVertPos);
          vec3 halfway_vector =
              normalize(light_direction + view_direction);

          float distance    = length(UNIFORM(light_point_pos)[i] - vVertPos);
          float attenuation = 1.0 / (distance * distance);
          vec3 radiance = UNIFORM(light_point_color)[i] * attenuation;

          float n_dot_l = max(dot(normal, light_direction), 0.0);
          float n_dot_h = max(dot(normal, halfway_vector), 0.0);
          float v_dot_h = max(dot(view_direction, halfway_vector), 0.0);

          // Calculate the terms for the microfacet specular shading model.
          vec3 fresnel_term = FresnelLerp(specular_environment_R0,
                                          specular_environment_R90,
                                          v_dot_h);
          float geometric_term = GeometricOcclusionEpic(n_dot_l, n_dot_v,
                                                        alpha_roughness);
          float normal_distribution =
              DistributionTrowbridgeReitz(alpha_roughness, n_dot_h);

          vec3 diffuse_contrib = (1.0 - fresnel_term) *
            LambertianDiffuse(diffuse_color);

          // Apply Torrance-Sparrow BRDF:
          //    D * F * G
          // ---------------
          //  4 * n.L * n.V
          float torrance_sparrow_divisor = 4.0 * n_dot_l * n_dot_v;
          vec3 spec_contrib = fresnel_term * geometric_term *
                              normal_distribution / torrance_sparrow_divisor;
          // If divisor is zero, zero out specular instead of dividing by zero.
          spec_contrib = torrance_sparrow_divisor ==
              0. ? vec3(0) : spec_contrib;

          // Obtain final intensity as reflectance (BRDF) scaled by the energy
          // of the light (cosine law)
          accumulated_light +=
              n_dot_l * radiance * (diffuse_contrib + spec_contrib);
        }
      |||,
    },
    {
      name: 'Spot Light',
      inputs: [{
        name: 'vViewDirection',
        type: 'Vec3f',
      }, {
        name: 'vVertPos',
        type: 'Vec3f',
      }],
      uniforms: [
        {
          name: 'light_spotlight_dir',
          type: 'Float3',
          array_size: 1,
          values: [0.0, 0.0, -1.0],
        },
        {
          name: 'light_spotlight_pos',
          type: 'Float3',
          array_size: 1,
          values: [0.0, 0.0, 0.0],
        },
        {
          name: 'light_spotlight_color',
          type: 'Float3',
          array_size: 1,
          values: [0.0, 0.0, 0.0,],
        },
        {
          name: 'light_spotlight_angle_cos',
          type: 'Float1',
          array_size: 1,
          values: [0.0],
        },
        {
          name: 'light_spotlight_penumbra_cos',
          type: 'Float1',
          array_size: 1,
          values: [0.0],
        },
        {
          name: 'light_spotlight_decay',
          type: 'Float1',
          array_size: 1,
          values: [2.0],
        },
      ],
      code: |||
        #include "lullaby/data/shaders/math.glslh"
        #include "lullaby/data/shaders/pbr/pbr_surface_common.glslh"

        #define MAX_SPOT_LIGHTS 1
      |||,
      main_code: |||
        for (int i = 0; i < MAX_SPOT_LIGHTS; ++i) {
          vec3 light_direction =
              normalize(UNIFORM(light_spotlight_pos)[i] - vVertPos);
          vec3 spot_direction = -UNIFORM(light_spotlight_dir)[i];
          float spotlight_cos = UNIFORM(light_spotlight_angle_cos)[i];
          float penumbra_cos = UNIFORM(light_spotlight_penumbra_cos)[i];

          float angle_cos = dot(light_direction, spot_direction);
          if (angle_cos > spotlight_cos) {
            float spot_effect =
                smoothstep(spotlight_cos, penumbra_cos, angle_cos);

            vec3 halfway_vector =
                normalize(light_direction + view_direction);

            float distance = length(UNIFORM(light_spotlight_pos)[i] - vVertPos);
            float attenuation = 1.0 / (distance * distance);
            vec3 radiance =
                UNIFORM(light_spotlight_color)[i] * attenuation * spot_effect;

            float n_dot_l = max(dot(normal, light_direction), 0.0);
            float n_dot_h = max(dot(normal, halfway_vector), 0.0);
            float v_dot_h = max(dot(view_direction, halfway_vector), 0.0);

            // Calculate the terms for the microfacet specular shading model.
            vec3 fresnel_term = FresnelLerp(specular_environment_R0,
                                            specular_environment_R90,
                                            v_dot_h);
            float geometric_term = GeometricOcclusionEpic(n_dot_l, n_dot_v,
                                                          alpha_roughness);
            float normal_distribution =
                DistributionTrowbridgeReitz(alpha_roughness, n_dot_h);

            vec3 diffuse_contrib = (1.0 - fresnel_term) *
              LambertianDiffuse(diffuse_color);

            // Apply Torrance-Sparrow BRDF:
            //    D * F * G
            // ---------------
            //  4 * n.L * n.V
            float torrance_sparrow_divisor = 4.0 * n_dot_l * n_dot_v;
            vec3 spec_contrib = fresnel_term * geometric_term *
                                normal_distribution / torrance_sparrow_divisor;
            // If divisor is zero, zero out specular instead of dividing by
            // zero.
            spec_contrib = torrance_sparrow_divisor ==
                0. ? vec3(0) : spec_contrib;

            // Obtain final intensity as reflectance (BRDF) scaled by the energy
            // of the light (cosine law)
            accumulated_light +=
                n_dot_l * radiance * (diffuse_contrib + spec_contrib);
          }
        }
      |||,
    },
  ],
}
