{
  shaders: [{
    name: 'basic',
    shading: 'Lit',
    vertex_attributes: ['Position', 'Orientation', 'TexCoord0'],
    blending: 'Opaque',
    defines: ['HAS_BASE_COLOR_MAP'],
    parameters: [{
      name: 'BaseTransform',
      type: 'Float4x4',
      default_floats: [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1],
    }, {
      name: 'BaseColor',
      type: 'Float4',
      default_floats: [1, 1, 1, 1],
    }, {
      name: 'Metallic',
      type: 'Float1',
      default_floats: [0],
    }, {
      name: 'Roughness',
      type: 'Float1',
      default_floats: [1],
    }, {
      name: 'EmissiveFactor',
      type: 'Float4',
      default_floats: [0, 0, 0, 0],
    }, {
      name: 'BaseColorSampler',
      type: 'Sampler2D',
      texture_usage: ['BaseColor'],
    }],
    fragment_shader: 'redux/data/shaders/glsl/fragment_pbr.glsl',
  }, {
    name: 'color-only',
    shading: 'Lit',
    vertex_attributes: ['Position', 'Orientation'],
    blending: 'Opaque',
    parameters: [{
      name: 'BaseTransform',
      type: 'Float4x4',
      default_floats: [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1],
    }, {
      name: 'BaseColor',
      type: 'Float4',
      default_floats: [1, 1, 1, 1],
    }, {
      name: 'Metallic',
      type: 'Float1',
      default_floats: [0],
    }, {
      name: 'Roughness',
      type: 'Float1',
      default_floats: [1],
    }, {
      name: 'EmissiveFactor',
      type: 'Float4',
      default_floats: [0, 0, 0, 0],
    }],
    fragment_shader: 'redux/data/shaders/glsl/fragment_pbr.glsl',
  }],
}
