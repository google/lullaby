local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [{
    name: 'Depth',
    main_code: |||
      gl_FragColor = vec4(gl_FragCoord.zzz, 1);
    |||,
  }],
}
