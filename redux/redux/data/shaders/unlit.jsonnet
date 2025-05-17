{
  shaders: [{
    name: 'unlit+color_map',
    shading: 'Unlit',
    defines: ['HAS_BASECOLOR_MAP'],
    vertex_attributes: ['Position', 'TexCoord0'],
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
      name: 'BaseColor',
      type: 'Sampler2D',
      texture_usage: ['BaseColor'],
    }],
    vertex_shader: 'redux/data/shaders/glsl/vertex_pbr.glsl',
    fragment_shader: 'redux/data/shaders/glsl/fragment_unlit.glsl',
  }, {
    name: 'unlit',
    shading: 'Unlit',
    vertex_attributes: ['Position'],
    blending: 'Opaque',
    parameters: [{
      name: 'BaseTransform',
      type: 'Float4x4',
      default_floats: [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1],
    }, {
      name: 'BaseColor',
      type: 'Float4',
      default_floats: [1, 1, 1, 1],
    }],
    vertex_shader: 'redux/data/shaders/glsl/vertex_pbr.glsl',
    fragment_shader: 'redux/data/shaders/glsl/fragment_unlit.glsl',
  }],
}
