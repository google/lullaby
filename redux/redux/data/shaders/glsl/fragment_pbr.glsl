#ifdef HAS_BASE_COLOR_MAP
vec2 getBaseColorUv() {
  return getUV0();
}
vec4 sampleBaseColor() {
  return texture(materialParams_BaseColorSampler, getBaseColorUv());
}
#endif

#ifdef HAS_RM_MAP
vec2 getRoughnessMetallicUv() {
  return getUV0();
}
vec4 sampleRoughnessMetallic() {
  return texture(materialParams_RoughnessMetallicSampler, getRoughnessMetallicUv());
}
#endif

#ifdef HAS_AO_MAP
vec2 getOcclusionUv() {
  return getUV0();
}
vec4 sampleOcclusion() {
  return texture(materialParams_AmbientOcclusionSampler, getOcclusionUv());
}
#endif

#ifdef HAS_ORM_MAP
vec2 getOcclusionRoughnessMetallicUv() {
  return getUV0();
}
vec4 sampleOcclusionRoughnessMetallic() {
  return texture(materialParams_OcclusionRoughnessMetallicSampler, getOcclusionRoughnessMetallicUv());
}
#endif

#ifdef HAS_NORMAL_MAP
vec2 getNormalUv() {
  return getUV0();
}
vec3 sampleNormal() {
  return texture(materialParams_NormalSampler, getNormalUv()).xyz;
}
#endif

#ifdef HAS_EMISSIVE_MAP
vec2 getEmissiveUv() {
  return getUV0();
}
vec3 sampleEmissive() {
  return texture(materialParams_EmissiveSampler, getEmissiveUv()).xyz;
}
#endif

void material(inout MaterialInputs material) {
#ifdef HAS_NORMAL_MAP
  vec3 normal_scale = vec3(materialParams.NormalScale, materialParams.NormalScale, 1);
  material.normal = (sampleNormal() * 2.0 - 1.0) * normal_scale;
#endif

  prepareMaterial(material);
  material.baseColor = materialParams.BaseColor;
  material.metallic = materialParams.Metallic;
  material.roughness = materialParams.Roughness;
  material.emissive = materialParams.EmissiveFactor;

#ifdef HAS_BASE_COLOR_MAP
  material.baseColor *= sampleBaseColor();
#endif

#ifdef HAS_ORM_MAP
  vec4 orm = sampleOcclusionRoughnessMetallic();
  material.ambientOcclusion = 1.0 + materialParams.AmbientOcclusionStrength * (orm.r - 1.0);
  material.roughness *= orm.g;
  material.metallic *= orm.b;
#else
#ifdef HAS_AO_MAP
  vec4 ao = sampleOcclusion();
  material.ambientOcclusion = 1.0 + materialParams.AmbientOcclusionStrength * (ao.r - 1.0);
#endif
#ifdef HAS_RM_MAP
  vec4 rm = sampleRoughnessMetallic();
  material.roughness *= rm.g;
  material.metallic *= rm.b;
#endif
#endif

#ifdef HAS_EMISSIVE_MAP
  material.emissive.rgb = sampleEmissive();
#endif
}
