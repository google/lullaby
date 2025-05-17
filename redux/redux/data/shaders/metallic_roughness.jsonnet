{
  shaders: [{
    name: 'lit',
    shading: 'Lit',
    vertex_attributes: ['Position', 'Orientation'],
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
      default_floats: [1],
    }, {
      name: 'Roughness',
      type: 'Float1',
      default_floats: [1],
    }, {
      name: 'EmissiveFactor',
      type: 'Float4',
      default_floats: [0, 0, 0, 0],
    }],
    vertex_shader: 'redux/data/shaders/glsl/vertex_pbr.glsl',
    fragment_shader: 'redux/data/shaders/glsl/fragment_pbr.glsl',
  }, {
    name: 'transparent',
    blending: 'Transparent',
    features: ['Transparent'],
    transparency: 'TwoPassesOneSide',
  }, {
    name: 'double_sided',
    double_sided: true,
    features: ['DoubleSided'],
    transparency: 'TwoPassesTwoSides',
  }, {
    name: 'color_map',
    defines: ['HAS_BASE_COLOR_MAP'],
    vertex_attributes: ['TexCoord0'],
    parameters: [{
      name: 'BaseColorSampler',
      type: 'Sampler2D',
      texture_usage: ['BaseColor'],
    }],
  }, {
    name: 'normal_map',
    defines: ['HAS_NORMAL_MAP'],
    vertex_attributes: ['TexCoord0'],
    parameters: [{
      name: 'NormalScale',
      type: 'Float1',
      default_floats: [1],
    }, {
      name: 'NormalSampler',
      type: 'Sampler2D',
      texture_usage: ['Normal', 'Normal', 'Normal'],
    }],
  }, {
    name: 'occlusion_roughness_metallic_map',
    defines: ['HAS_ORM_MAP'],
    vertex_attributes: ['TexCoord0'],
    parameters: [{
      name: 'OcclusionRoughnessMetallicSampler',
      type: 'Sampler2D',
      texture_usage: ['Occlusion', 'Roughness', 'Metallic'],
    }, {
      name: 'AmbientOcclusionStrength',
      type: 'Float1',
      default_floats: [1],
    }]
  }, {
    name: 'roughness_metallic_map',
    defines: ['HAS_RM_MAP'],
    vertex_attributes: ['TexCoord0'],
    parameters: [{
      name: 'RoughnessMetallicSampler',
      type: 'Sampler2D',
      texture_usage: ['Unspecified', 'Roughness', 'Metallic'],
    }]
  }, {
    name: 'occlusion_map',
    defines: ['HAS_AO_MAP'],
    vertex_attributes: ['TexCoord0'],
    parameters: [{
      name: 'AmbientOcclusionSampler',
      type: 'Sampler2D',
      texture_usage: ['Occlusion', 'Unspecified', 'Unspecified'],
    }, {
      name: 'AmbientOcclusionStrength',
      type: 'Float1',
      default_floats: [1],
    }],
  }, {
    name: 'emissive_map',
    defines: ['HAS_EMISSIVE_MAP'],
    vertex_attributes: ['TexCoord0'],
    parameters: [{
      name: 'EmissiveSampler',
      type: 'Sampler2D',
      texture_usage: ['Emissive'],
    }]
  }],
  variants: [
    ['lit', 'transparent', 'double_sided', 'color_map', 'normal_map', 'occlusion_roughness_metallic_map',                                            'emissive_map'],
    ['lit',                'double_sided', 'color_map', 'normal_map', 'occlusion_roughness_metallic_map',                                            'emissive_map'],
    ['lit', 'transparent',                 'color_map', 'normal_map', 'occlusion_roughness_metallic_map',                                            'emissive_map'],
    ['lit',                                'color_map', 'normal_map', 'occlusion_roughness_metallic_map',                                            'emissive_map'],
    ['lit', 'transparent', 'double_sided', 'color_map', 'normal_map',                                     'roughness_metallic_map', 'occlusion_map', 'emissive_map'],
    ['lit',                'double_sided', 'color_map', 'normal_map',                                     'roughness_metallic_map', 'occlusion_map', 'emissive_map'],
    ['lit', 'transparent',                 'color_map', 'normal_map',                                     'roughness_metallic_map', 'occlusion_map', 'emissive_map'],
    ['lit',                                'color_map', 'normal_map',                                     'roughness_metallic_map', 'occlusion_map', 'emissive_map'],
    ['lit', 'transparent', 'double_sided', 'color_map', 'normal_map',                                     'roughness_metallic_map',                  'emissive_map'],
    ['lit',                'double_sided', 'color_map', 'normal_map',                                     'roughness_metallic_map',                  'emissive_map'],
    ['lit', 'transparent',                 'color_map', 'normal_map',                                     'roughness_metallic_map',                  'emissive_map'],
    ['lit',                                'color_map', 'normal_map',                                     'roughness_metallic_map',                  'emissive_map'],
    ['lit', 'transparent', 'double_sided', 'color_map', 'normal_map', 'occlusion_roughness_metallic_map'],
    ['lit',                'double_sided', 'color_map', 'normal_map', 'occlusion_roughness_metallic_map'],
    ['lit', 'transparent',                 'color_map', 'normal_map', 'occlusion_roughness_metallic_map'],
    ['lit',                                'color_map', 'normal_map', 'occlusion_roughness_metallic_map'],
    ['lit', 'transparent', 'double_sided', 'color_map', 'normal_map',                                     'roughness_metallic_map', 'occlusion_map'],
    ['lit',                'double_sided', 'color_map', 'normal_map',                                     'roughness_metallic_map', 'occlusion_map'],
    ['lit', 'transparent',                 'color_map', 'normal_map',                                     'roughness_metallic_map', 'occlusion_map'],
    ['lit',                                'color_map', 'normal_map',                                     'roughness_metallic_map', 'occlusion_map'],
    ['lit', 'transparent', 'double_sided', 'color_map', 'normal_map',                                     'roughness_metallic_map'],
    ['lit',                'double_sided', 'color_map', 'normal_map',                                     'roughness_metallic_map'],
    ['lit', 'transparent',                 'color_map', 'normal_map',                                     'roughness_metallic_map'],
    ['lit',                                'color_map', 'normal_map',                                     'roughness_metallic_map'],
    ['lit', 'transparent', 'double_sided', 'color_map', 'normal_map'],
    ['lit',                'double_sided', 'color_map', 'normal_map'],
    ['lit', 'transparent',                 'color_map', 'normal_map'],
    ['lit',                                'color_map', 'normal_map'],
    ['lit', 'transparent', 'double_sided', 'color_map'],
    ['lit',                'double_sided', 'color_map'],
    ['lit', 'transparent',                 'color_map'],
    ['lit',                                'color_map'],
    ['lit', 'transparent', 'double_sided', ],
    ['lit',                'double_sided', ],
    ['lit', 'transparent',                 ],
    ['lit',                                ],
  ]
}