#version 450

// Vertex inputs
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inColor;
layout(location = 4) in uvec4 inBoneIndices;  // 4 bone indices as unsigned ints
layout(location = 5) in vec4 inBoneWeights;   // 4 bone weights

// Outputs to fragment shader
layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragWorldPos;

// Push constants for per-object data
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProjection;
    vec4 baseColor;
    vec4 lightDir;
    vec4 lightColor;
    vec4 ambientColor;
} pc;

// Bone matrices (max 100 bones) - uses descriptor set binding
layout(set = 0, binding = 0) uniform BoneMatrices {
    mat4 bones[100];
} boneData;

void main() {
    // Calculate skinned position by blending bone transforms
    mat4 skinMatrix = mat4(0.0);

    // Accumulate bone influence
    for (int i = 0; i < 4; ++i) {
        if (inBoneWeights[i] > 0.0) {
            skinMatrix += boneData.bones[inBoneIndices[i]] * inBoneWeights[i];
        }
    }

    // If no bone weights, use identity (unskinned vertex)
    float totalWeight = inBoneWeights[0] + inBoneWeights[1] + inBoneWeights[2] + inBoneWeights[3];
    if (totalWeight < 0.001) {
        skinMatrix = mat4(1.0);
    }

    // Apply skinning then model transform
    vec4 skinnedPos = skinMatrix * vec4(inPosition, 1.0);
    vec4 worldPos = pc.model * skinnedPos;

    fragWorldPos = worldPos.xyz;

    // Transform normal with skinning (use upper-left 3x3 of skin matrix)
    vec3 skinnedNormal = mat3(skinMatrix) * inNormal;
    fragNormal = mat3(pc.model) * skinnedNormal;

    fragTexCoord = inTexCoord;

    gl_Position = pc.viewProjection * worldPos;
}
