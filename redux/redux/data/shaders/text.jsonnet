{
  shaders: [{
    name: 'normal',
    shading: 'Unlit',
    defines: ['HAS_VERTEX_COLOR0'],
    vertex_attributes: ['Position', 'Color0', 'TexCoord0'],
    blending: 'Transparent',
    parameters: [{
      name: 'BaseColor',
      type: 'Float4',
      default_floats: [1, 1, 1, 1],
    }, {
      name: 'GlyphMap',
      type: 'Sampler2D',
      texture_usage: ['Glyph'],
    }],
    fragment_shader: 'redux/data/shaders/glsl/fragment_text.glsl',
  }, {
    name: 'sdf',
    shading: 'Unlit',
    vertex_attributes: ['Position', 'Color0', 'TexCoord0'],
    blending: 'Transparent',
    parameters: [{
      name: 'BaseColor',
      type: 'Float4',
      default_floats: [1, 1, 1, 1],
    }, {
      name: 'GlyphMap',
      type: 'Sampler2D',
      texture_usage: ['Glyph'],
    }, {
      name: 'SdfParams',
      type: 'Float4',
    }],
    defines: ['SDF_TEXT', 'HAS_VERTEX_COLOR0'],
    features: ['SDF_TEXT'],
    fragment_shader: 'redux/data/shaders/glsl/fragment_text.glsl',
  }],
}