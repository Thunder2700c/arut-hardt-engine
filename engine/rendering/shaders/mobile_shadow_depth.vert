#version 450

// Directional shadow-map depth pass. Shares the PBR instance stream.
layout(location = 0) in vec3 inPosition;
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
    mat4 boneMatrices[20];
} skinningData;

layout(push_constant) uniform ShadowCascadeData {
    uint cascadeIndex;
} shadowCascade;

void main() {
    mat4 model = mat4(inModelColumn0, inModelColumn1, inModelColumn2, inModelColumn3);
    vec4 weights = max(inBoneWeights, vec4(0.0));
    float weightSum = dot(weights, vec4(1.0));
    mat4 skin = mat4(1.0);
    if (weightSum > 0.00001) {
        weights /= weightSum;
        uvec4 joints = min(inBoneIndices, uvec4(19u));
        skin = skinningData.boneMatrices[joints.x] * weights.x
             + skinningData.boneMatrices[joints.y] * weights.y
             + skinningData.boneMatrices[joints.z] * weights.z
             + skinningData.boneMatrices[joints.w] * weights.w;
    }
    gl_Position = frameData.lightViewProjection[shadowCascade.cascadeIndex] *
                  model * skin * vec4(inPosition, 1.0);
}
