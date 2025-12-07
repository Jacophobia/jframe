#version 410 core

// Skinned/Skeletal Animation Vertex Shader
// Supports up to 4 bone influences per vertex

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;
layout(location = 4) in ivec4 aBoneIDs;     // Bone indices
layout(location = 5) in vec4 aBoneWeights;  // Bone weights

const int MAX_BONES = 100;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uBoneTransforms[MAX_BONES];

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vColor;

void main() {
    // Calculate skinned position
    mat4 boneTransform = uBoneTransforms[aBoneIDs[0]] * aBoneWeights[0];
    boneTransform += uBoneTransforms[aBoneIDs[1]] * aBoneWeights[1];
    boneTransform += uBoneTransforms[aBoneIDs[2]] * aBoneWeights[2];
    boneTransform += uBoneTransforms[aBoneIDs[3]] * aBoneWeights[3];

    vec4 skinnedPos = boneTransform * vec4(aPosition, 1.0);
    vec3 skinnedNormal = mat3(boneTransform) * aNormal;

    // Transform to world space
    vec4 worldPos = uModel * skinnedPos;
    vWorldPos = worldPos.xyz;
    vNormal = normalize(mat3(uModel) * skinnedNormal);
    vTexCoord = aTexCoord;
    vColor = aColor;

    gl_Position = uProjection * uView * worldPos;
}
