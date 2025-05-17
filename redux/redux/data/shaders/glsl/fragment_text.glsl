vec4 sampleGlyph() {
  return texture(materialParams_GlyphMap, getUV0());
}

void material(inout MaterialInputs material) {
  prepareMaterial(material);

  float mask = sampleGlyph().r;

#ifdef SDF_TEXT
    // sdf params:
    // x: dist offset, y: dist scale, z: dist @ a = 0
    // w: dist @ a = 1 (NB: w > z!)
    // For black glyphs in white space: x = 1, y = -1.
    vec4 sdf_params = materialParams.SdfParams;
    float dist = sdf_params.x + sdf_params.y * mask;
    if (dist < sdf_params.z) {
      discard;
    }
    float alpha = smoothstep(sdf_params.z, sdf_params.w, dist);
#else
    float alpha = mask;
#endif

  #ifdef HAS_VERTEX_COLOR0
  material.baseColor = getColor();
  #else
  material.baseColor = vec4(1, 1, 1, 1);
  #endif
  material.baseColor *= materialParams.BaseColor;
  material.baseColor.a = alpha;
  material.baseColor.rgb *= material.baseColor.a;
}