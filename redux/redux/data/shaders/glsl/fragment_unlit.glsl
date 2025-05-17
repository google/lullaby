#ifdef HAS_BASECOLOR_MAP
vec4 sampleBaseColor() {
  return texture(materialParams_BaseColor, getUV0());
}
#endif

void material(inout MaterialInputs material) {
  prepareMaterial(material);
  material.baseColor = materialParams.BaseColor;
  #ifdef HAS_BASECOLOR_MAP
  material.baseColor *= sampleBaseColor();
  #endif
}