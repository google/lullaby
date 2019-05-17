local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  components: [
    {
      def_type: 'TransformDef',
      def: {
        position: {
          x: 0.0,
          y: 0.0,
          z: -1.0,
        },
        scale: {
          x: 0.01,
          y: 0.01,
          z: 0.01,
        },
      },
    },
    {
      def_type: 'ModelAssetDef',
      def: {
        pass: utils.hash('Opaque'),
        filename: 'crabby.lullmodel',
        materials: [{
          shading_model: 'pbr',
        }],
      },
    },
  ],
}
