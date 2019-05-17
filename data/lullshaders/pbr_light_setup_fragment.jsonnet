local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Light Setup',
      inputs: [{
        name: 'vViewDirection',
        type: 'Vec3f',
      }],
      code: |||
        #include "lullaby/data/shaders/math.glslh"
        #include "lullaby/data/shaders/pbr/pbr_surface_common.glslh"

        // Fresnel reflectance for normal incidence for a diffuse material
        const vec3 kDiffuseFresnel0 = vec3(0.04);

        // Variables needed for light calculations.
        vec3 view_direction;
        vec3 reflection;
        float n_dot_v;

        vec3 diffuse_color;
        vec3 specular_color;

        vec3 specular_environment_R0;
        vec3 specular_environment_R90;
        float alpha_roughness;

        // All light snippets should add to this variable.
        vec3 accumulated_light;
      |||,
      main_code: |||
        accumulated_light = vec3(0.0);

        // Prepare light parameters.
        view_direction = normalize(vViewDirection);
        reflection = -normalize(reflect(view_direction, normal));
        n_dot_v = abs(dot(normal, view_direction));

        diffuse_color = base_color.rgb * (vec3(1.0) - kDiffuseFresnel0);
        diffuse_color *= 1.0 - metallic;
        specular_color = mix(kDiffuseFresnel0, base_color.rgb, metallic);

        // Compute reflectance.
        float reflectance = max(max(specular_color.r, specular_color.g),
                                specular_color.b);

        // For typical incident reflectance range (between 4% to 100%) set the
        // grazing reflectance to 100% for typical fresnel effect. For very low
        // reflectance range on highly diffuse objects (below 4%), incrementally
        // reduce grazing reflecance to 0%.
        float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
        specular_environment_R0 = specular_color.rgb;
        specular_environment_R90 = vec3(reflectance90);

        // Metallic and Roughness material properties are packed together
        // In glTF, these factors can be specified by fixed scalar values
        // or from a metallic-roughness map
        // Roughness is authored as perceptual roughness; as is convention,
        // convert to material roughness by squaring the perceptual roughness.
        alpha_roughness = roughness * roughness;
      |||,
    }
  ],
}
