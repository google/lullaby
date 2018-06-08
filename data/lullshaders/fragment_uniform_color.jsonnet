local utils = import 'third_party/lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [{
    name: "Uniform Color",
    features: [utils.hash('UniformColor')],
    uniforms: [{
      name: 'color',
      type: 'Float4',
    }],
    code: |||
      #include "third_party/lullaby/data/lullshaders/include/premultiply_alpha.glslh"
    |||,
    main_code: |||
      gl_FragColor *= PremultiplyAlpha(color);
    |||,
  }],
}
