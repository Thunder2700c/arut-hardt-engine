#version 450

// Directional shadow-map depth pass. Shares the PBR instance stream.
layout(location = 0) in vec3 inPosition;
layout(location = 4) in vec4 inModelColumn0;
layout(location = 5) in vec4 inModelColumn1;
layout(location = 6) in vec4 inModelColumn2;
layout(location = 7) in vec4 inModelColumn3;

layout(set = 0, binding = 0, std140) uniform FrameData {
    mat4 viewProjection;
    vec4 cameraPosition;
    vec4 directionalLightDirection;
    vec4 directionalLightRadiance;
    mat4 lightViewProjection;
} frameData;

void main() {
    mat4 model = mat4(inModelColumn0, inModelColumn1, inModelColumn2, inModelColumn3);
    gl_Position = frameData.lightViewProjection * model * vec4(inPosition, 1.0);
}
