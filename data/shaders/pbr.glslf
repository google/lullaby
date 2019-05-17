// Samples texture 0 and scales by a "color" uniform.

#include "lullaby/data/shaders/compatibility.glslh"

#if !defined(LULLABY_PBR_FIBL_AVAILABLE)
#define LULLABY_PBR_FIBL_AVAILABLE 0
#endif  // !defined(LULLABY_PBR_FIBL_AVAILABLE)

#if !defined(LULLABY_PBR_SCREEN_SPACE_REFL)
#define LULLABY_PBR_SCREEN_SPACE_REFL 0
#endif  // !defined(LULLABY_PBR_SCREEN_SPACE_REFL)

#if !defined(LULLABY_PBR_TRANSPARENCY_SUPPORTED)
#define LULLABY_PBR_TRANSPARENCY_SUPPORTED 0
#endif  // !defined(LULLABY_PBR_TRANSPARENCY_SUPPORTED)

#include "lullaby/data/shaders/uber_fragment_common.glslh"
#include "lullaby/data/shaders/math.glslh"
#include "lullaby/data/shaders/pbr/pbr_surface_env.glslh"
#include "lullaby/data/shaders/pbr/gamma.glslh"

uniform vec4 BaseColor;
uniform sampler2D texture_unit_0;  // base color texture
uniform sampler2D texture_unit_1;  // occlusion roughness metallic
uniform sampler2D texture_unit_2;  // normal map
uniform samplerCube texture_unit_16;  // diffuse environment map
// TODO Get specular env map working.
// uniform samplerCube texture_unit_17;  // specular environment map
                                        // (already defined)
uniform sampler2D texture_unit_15;  // brdf look-up table

STAGE_INPUT vec3 vViewDirection;
STAGE_INPUT mat3 vTangentBitangentNormal;

const float kMinRoughness = 0.04;
const float kEpsilon = 0.001;

// Fresnel reflectance for normal incidence for a diffuse material
const vec3 kDiffuseFresnel0 = vec3(0.04);


// Calculates the lighting contribution from an Image Based Light source.
// Precomputed Environment Maps are computed as outlined in
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
vec3 IblContribution(float perceptual_roughness, float n_dot_v,
                     vec3 diffuse_color, vec3 specular_color, vec3 normal,
                     vec3 reflection, vec2 ibl_light_scale) {
    vec2 lookup_uv = vec2(n_dot_v, 1.0 - perceptual_roughness);
  vec2 brdf_raw = texture2D(texture_unit_15, lookup_uv).rg;
  vec2 brdf = RemoveGamma(brdf_raw);
  vec3 diffuse_light = RemoveGamma(textureCube(texture_unit_16, normal)).rgb;
// TODO Get specular env map working.
  // vec3 specular_light = RemoveGamma(textureCube(texture_unit_17,
  //                                               reflection)).rgb;

  vec3 diffuse = diffuse_light * diffuse_color;
  // vec3 specular = specular_light * (specular_color * brdf.x + brdf.y);

  diffuse *= ibl_light_scale.x;
  // specular *= ibl_light_scale.y;

  return diffuse;// + specular;
}


void main() {
  vec4 raw_base_color = texture2D(texture_unit_0, vTexCoord);
  // TODO Set BaseColor uniform in the gltf pbr shader.
  // Currently it is not set, so using it will give us black.  this should be
  // defaulted to 1.
  // vec4 base_color = BaseColor * RemoveGamma(raw_base_color.rgba);
  vec4 base_color = RemoveGamma(raw_base_color.rgba);
  vec3 orm = texture2D(texture_unit_1, vTexCoord).xyz;
  vec3 material_attribs = vec3(1, 1, 1); // TODO: load from uniform
  float occlusion = orm.x * material_attribs.x;
  float roughness = orm.y * material_attribs.y;
  float metallic = orm.z * material_attribs.z;
  vec3 raw_normal = texture2D(texture_unit_2, vTexCoord).xyz;
  vec3 view_direction = normalize(vViewDirection);
  vec3 normal = normalize(vTangentBitangentNormal * (2. * raw_normal - 1.));
  vec3 light_direction = normalize(vec3(1, 1, 1));
  vec3 light_color = vec3(1, 1, 1) * 2.0; // TODO: load from uniform
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

  // For typical incident reflectance range (between 4% to 100%) set the grazing
  // reflectance to 100% for typical fresnel effect. For very low reflectance
  // range on highly diffuse objects (below 4%), incrementally reduce grazing
  // reflecance to 0%.
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
  vec3 spec_contrib = fresnel_term * geometric_term * normal_distribution /
                      (4.0 * n_dot_l * n_dot_v);
  // Obtain final intensity as reflectance (BRDF) scaled by the energy of the
  // light (cosine law)
  vec3 color = n_dot_l * light_color * (diffuse_contrib + spec_contrib);

  vec2 ibl_light_scale = vec2(1.0, 1.0); // Todo: load from uniform
  color += IblContribution(perceptual_roughness, n_dot_v, diffuse_color,
                           specular_color, normal, reflection, ibl_light_scale);

  // Apply gamma correction and premultiply alpha.  Note that we do not want
  // to be doing gamma correction in the shader, but for this we would need to
  // have an sRGB or other gamma correcting render target.  Android O and less
  // do not expose this.
  vec3 gamma_correct = ApplyGamma(color);
  SetFragColor(vec4(gamma_correct * base_color.a, base_color.a));
}
