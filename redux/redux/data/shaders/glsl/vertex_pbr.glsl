void materialVertex(inout MaterialVertexInputs material) {
  mat4 model_matrix = getWorldFromModelMatrix() * materialParams.BaseTransform;
  material.worldPosition = mulMat4x4Float3(model_matrix, getPosition().xyz);
}
