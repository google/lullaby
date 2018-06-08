local utils = import 'third_party/lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Skinned Transform',
      features: [utils.hash('Transform')],
      inputs: [{
        name: 'aPosition',
        type: 'Vec4f',
        usage: 'Position',
      }, {
        name: 'aBoneIndices',
        type: 'Vec4f',
        usage: 'BoneIndices',
      }, {
        name: 'aBoneWeights',
        type: 'Vec4f',
        usage: 'BoneWeights',
      }],
      uniforms: [{
        name: 'model_view_projection',
        type: 'Float4x4',
      }, {
        name: 'bone_transforms',
        type: 'Float4',
        // Array size needs to be large enough to support the number of bones
        // in meshes, but also satisfy hardware constraints.
        //
        // 512 was selected (even though it isn't divisable by 3) because of
        // hardware constraints and large number of bones.
        array_size: 512,
      }],
      code: |||
        #include "third_party/lullaby/data/lullshaders/include/skinning.glslh"
      |||,
      main_code: |||
        vec4 skinned_position =
            FourBoneSkinnedVertex(aBoneIndices, aBoneWeights, aPosition);
        gl_Position = model_view_projection * skinned_position;
      |||,
    },
    {
      name: 'Skinned Multiview Transform',
      features: [utils.hash('Transform')],
      environment: [utils.hash('MULTIVIEW')],
      inputs: [{
        name: 'aPosition',
        type: 'Vec4f',
        usage: 'Position',
      }, {
        name: 'aBoneIndices',
        type: 'Vec4f',
        usage: 'BoneIndices',
      }, {
        name: 'aBoneWeights',
        type: 'Vec4f',
        usage: 'BoneWeights',
      }],
      uniforms: [{
        name: 'model_view_projection',
        type: 'Float4x4',
        array_size: 2,
      }, {
        name: 'bone_transforms',
        type: 'Float4',
        // Array size needs to be large enough to support the number of bones
        // in meshes, but also satisfy hardware constraints.
        //
        // 512 was selected (even though it isn't divisable by 3) because of
        // hardware constraints and large number of bones.
        array_size: 512,
      }],
      code: |||
        #extension GL_OVR_multiview2 : require
        layout(num_views = 2) in;
        #include "third_party/lullaby/data/lullshaders/include/skinning.glslh"
      |||,
      main_code: |||
        vec4 skinned_position =
            FourBoneSkinnedVertex(aBoneIndices, aBoneWeights, aPosition);
        gl_Position = model_view_projection[gl_ViewID_OVR] * aPosition;
      |||,
    },
  ],
}
