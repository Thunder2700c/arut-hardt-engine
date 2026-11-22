#version 450

// ARUT-HARDT mobile forward PBR vertex stage.
// Vertex attribute locations: position 0, normal 1, tangent 2, UV 3.

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inUv;
layout(location = 4) in vec4 inModelColumn0;
layout(location = 5) in vec4 inModelColumn1;
layout(location = 6) in vec4 inModelColumn2;
layout(location = 7) in vec4 inModelColumn3;

layout(set = 0, binding = 0, std140) uniform FrameData {
    mat4 viewProjection;
    vec4 cameraPosition;
    vec4 directionalLightDirection;
    vec4 directionalLightRadiance;
} frameData;

layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec3 outWorldNormal;
layout(location = 2) out vec4 outWorldTangent;
layout(location = 3) out vec2 outUv;

void main() {
    mat4 model = mat4(inModelColumn0, inModelColumn1, inModelColumn2, inModelColumn3);
    vec4 worldPosition = model * vec4(inPosition, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    outWorldPosition = worldPosition.xyz;
    outWorldNormal = normalize(normalMatrix * inNormal);
    outWorldTangent = vec4(normalize(mat3(objectData.model) * inTangent.xyz), inTangent.w);
    outUv = inUv;
    gl_Position = frameData.viewProjection * worldPosition;
}
