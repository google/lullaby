local utils = import 'third_party/lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: "Physically Based Rendering",
      inputs: [{
        name: 'vTexCoord',
        type: 'Vec2f',
      }, {
        name: 'vViewDirection',
        type: 'Vec3f',
      }],
      code: |||
        #include "third_party/lullaby/data/lullshaders/include/premultiply_alpha.glslh"
        #include "third_party/lullaby/data/shaders/math.glslh"
        #include "third_party/lullaby/data/shaders/pbr/pbr_surface_common.glslh"
        #include "third_party/lullaby/data/shaders/pbr/gamma.glslh"

        const float kMinRoughness = 0.04;
        const float kEpsilon = 0.001;

        // Fresnel reflectance for normal incidence for a diffuse material
        const vec3 kDiffuseFresnel0 = vec3(0.04);
      |||,
      main_code: |||
        vec3 view_direction = normalize(vViewDirection);

        // TODO(b/78245674): Expose these constants outside the shader
        vec3 light_direction = normalize(vec3(1, 1, 1));
        vec3 light_color = vec3(1, 1, 1) * 2.0;
        vec3 halfway_vector = normalize(light_direction + view_direction);
        vec3 reflection = -normalize(reflect(view_direction, normal));

        float n_dot_l = clamp(dot(normal, light_direction), kEpsilon, 1.0);
        float n_dot_v = abs(dot(normal, view_direction)) + 0.001;
        float n_dot_h = clamp(dot(normal, halfway_vector), 0.0, 1.0);
        float v_dot_h = clamp(dot(view_direction, halfway_vector), 0.0, 1.0);

        vec3 diffuse_color = base_color.rgb * (vec3(1.0) - kDiffuseFresnel0);
        diffuse_color *= 1.0 - metallic;
        vec3 specular_color = mix(kDiffuseFresnel0, base_color.rgb, metallic);

        // Compute reflectance.
        float reflectance = max(max(specular_color.r, specular_color.g),
                                specular_color.b);

        // For typical incident reflectance range (between 4% to 100%) set the
        // grazing reflectance to 100% for typical fresnel effect. For very low
        // reflectance range on highly diffuse objects (below 4%), incrementally
        // reduce grazing reflecance to 0%.
        float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
        vec3 specular_environment_R0 = specular_color.rgb;
        vec3 specular_environment_R90 = vec3(reflectance90);

        // Metallic and Roughness material properties are packed together
        // In glTF, these factors can be specified by fixed scalar values
        // or from a metallic-roughness map
        float perceptual_roughness = roughness;
        perceptual_roughness = clamp(perceptual_roughness, kMinRoughness, 1.0);
        // Roughness is authored as perceptual roughness; as is convention,
        // convert to material roughness by squaring the perceptual roughness.
        float alpha_roughness = perceptual_roughness * perceptual_roughness;

        // Calculate the shading terms for the microfacet specular shading model
        vec3 fresnel_term = FresnelLerp(specular_environment_R0,
                                        specular_environment_R90,
                                        v_dot_h);
        float geometric_term = GeometricOcclusionEpic(n_dot_l, n_dot_v,
                                                      alpha_roughness);
        float normal_distribution = DistributionTrowbridgeReitz(alpha_roughness,
                                                                n_dot_h);

        // Calculation of analytical lighting contribution
        vec3 diffuse_contrib = (1.0 - fresnel_term) *
          LambertianDiffuse(diffuse_color);
        vec3 spec_contrib = fresnel_term * geometric_term *
                            normal_distribution / (4.0 * n_dot_l * n_dot_v);
        // Obtain final intensity as reflectance (BRDF) scaled by the energy of
        // the light (cosine law)
        vec3 color = n_dot_l * light_color * (diffuse_contrib + spec_contrib);

        vec3 gamma_correct = ApplyGamma(color);
        gl_FragColor = PremultiplyAlpha(vec4(gamma_correct, base_color.w));
      |||,
    },
  ],
}
