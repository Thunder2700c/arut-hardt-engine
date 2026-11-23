#version 450

// ARUT-HARDT mobile forward PBR vertex stage.
// Vertex attributes: P/N/T/UV 0-3, instance model 4-7, joint indices/weights 8-9.

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inUv;
layout(location = 4) in vec4 inModelColumn0;
layout(location = 5) in vec4 inModelColumn1;
layout(location = 6) in vec4 inModelColumn2;
layout(location = 7) in vec4 inModelColumn3;
layout(location = 8) in uvec4 inBoneIndices;
layout(location = 9) in vec4 inBoneWeights;

layout(set = 0, binding = 0, std140) uniform FrameData {
    mat4 viewProjection;
    vec4 cameraPosition;
    vec4 cameraForward;
    vec4 directionalLightDirection;
    vec4 directionalLightRadiance;
    mat4 lightViewProjection[4];
    vec4 cascadeSplits;
} frameData;

layout(set = 0, binding = 8, std140) uniform BonePalette {
    mat4 boneMatrices[128];
} skinningData;

layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec3 outWorldNormal;
layout(location = 2) out vec4 outWorldTangent;
layout(location = 3) out vec2 outUv;

void main() {
    mat4 model = mat4(inModelColumn0, inModelColumn1, inModelColumn2, inModelColumn3);
    vec4 weights = max(inBoneWeights, vec4(0.0));
    float weightSum = dot(weights, vec4(1.0));
    mat4 skin = mat4(1.0);
    if (weightSum > 0.00001) {
        weights /= weightSum;
        uvec4 joints = min(inBoneIndices, uvec4(127u));
        skin = skinningData.boneMatrices[joints.x] * weights.x
             + skinningData.boneMatrices[joints.y] * weights.y
             + skinningData.boneMatrices[joints.z] * weights.z
             + skinningData.boneMatrices[joints.w] * weights.w;
    }
    vec4 skinnedPosition = skin * vec4(inPosition, 1.0);
    vec3 skinnedNormal = mat3(skin) * inNormal;
    vec3 skinnedTangent = mat3(skin) * inTangent.xyz;
    vec4 worldPosition = model * skinnedPosition;
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    outWorldPosition = worldPosition.xyz;
    outWorldNormal = normalize(normalMatrix * skinnedNormal);
    outWorldTangent = vec4(normalize(mat3(model) * skinnedTangent), inTangent.w);
    outUv = inUv;
    gl_Position = frameData.viewProjection * worldPosition;
}
