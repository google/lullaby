local utils = import 'lullaby/data/jsonnet/utils.jsonnet';
{
  snippets: [
    {
      name: 'Skinned Transform',
      features: [utils.hash('Transform')],
      versions: [{
        lang: 'GL_Compat',
        min_version: 100,
        max_version: 300,
      }],
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
      },{
        name: 'model',
        type: 'Float4x4',
      },{
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
        // The skinning shader expects the uniform to be called
        // 'bone_transforms_array', but it is called bone_transforms. This is
        // because a UBO and field inside a UBO cannot share a name on some
        // compilers. Instead, we redirect the skinning code to the right field
        // by doing the following define. See skinning.glslh for more info.
        #define bone_transforms_array bone_transforms
        #include "lullaby/data/lullshaders/include/skinning.glslh"

        // This will be consumed by the view_direction snippet.
        vec3 world_position;
      |||,
      main_code: |||
        vec4 skinned_position =
            FourBoneSkinnedVertex(aBoneIndices, aBoneWeights, aPosition);
        gl_Position = UNIFORM(model_view_projection) * skinned_position;
        world_position = (UNIFORM(model) * skinned_position).xyz;
      |||,
    },
    {
      name: 'Skinned Transform',
      features: [utils.hash('Transform')],
      versions: [{
        lang: 'GL_Compat',
        min_version: 300,
        max_version: 0,
      }],
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
      },{
        name: 'model',
        type: 'Float4x4',
      },{
        name: 'camera_pos',
        type: 'Float3',
      },{
        name: 'bone_transforms',
        type: 'BufferObject',
        fields: [{
          name: 'bone_transforms_array',
          type: 'Float4',
          array_size: 512,
        }],
      }],
      code: |||
        #include "lullaby/data/lullshaders/include/skinning.glslh"

        // This will be consumed by the view_direction snippet.
        vec3 world_position;
      |||,
      main_code: |||
        vec4 skinned_position =
            FourBoneSkinnedVertex(aBoneIndices, aBoneWeights, aPosition);
        gl_Position = UNIFORM(model_view_projection) * skinned_position;
        world_position = (UNIFORM(model) * skinned_position).xyz;
      |||,
    },
  ],
}
