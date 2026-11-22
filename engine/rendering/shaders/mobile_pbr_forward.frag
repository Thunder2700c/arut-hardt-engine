#version 450

// ARUT-HARDT mobile forward PBR reference shader.
// Vertex inputs use glTF-style tangent-space conventions: +Y up in tangent space.
// All material/light colours are linear-light values.

layout(location = 0) in vec3 inWorldPosition;
layout(location = 1) in vec3 inWorldNormal;
layout(location = 2) in vec4 inWorldTangent;
layout(location = 3) in vec2 inUv;

layout(set = 0, binding = 0, std140) uniform FrameData {
    mat4 viewProjection;
    vec4 cameraPosition;
    vec4 directionalLightDirection;
    vec4 directionalLightRadiance;
    mat4 lightViewProjection;
} frameData;

layout(set = 0, binding = 1) uniform sampler2DShadow directionalShadowMap;
layout(set = 0, binding = 2) uniform samplerCube environmentMap;
layout(set = 0, binding = 3) uniform sampler2D brdfIntegrationLut;

layout(set = 1, binding = 0, std140) uniform MaterialData {
    vec4 baseColorFactor;
    vec4 emissiveMetallic;       // xyz emissive, w metallic
    vec4 roughnessNormalOcclusionAlpha; // x roughness, y normal scale, z occlusion, w alpha cutoff
    uvec4 materialFlags;         // bit 0 base-color map, bit 1 normal map, bit 2 metallic-roughness map, bit 3 alpha mask
} materialData;

layout(set = 1, binding = 1) uniform sampler2D baseColorMap;
layout(set = 1, binding = 2) uniform sampler2D normalMap;
layout(set = 1, binding = 3) uniform sampler2D metallicRoughnessMap;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265358979323846;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

vec3 fresnelSchlick(float cosTheta, vec3 f0) {
    return f0 + (1.0 - f0) * pow(1.0 - saturate(cosTheta), 5.0);
}

float distributionGGX(float nDotH, float roughness) {
    float a = max(0.045, roughness);
    float a2 = a * a * a * a;
    float d = nDotH * nDotH * (a2 - 1.0) + 1.0;
    return a2 / max(1e-7, PI * d * d);
}

float geometrySchlickGGX(float nDotV, float roughness) {
    float r = roughness + 1.0;
    float k = r * r * 0.125;
    return nDotV / max(1e-7, nDotV * (1.0 - k) + k);
}

float geometrySmith(float nDotV, float nDotL, float roughness) {
    return geometrySchlickGGX(nDotV, roughness) * geometrySchlickGGX(nDotL, roughness);
}

float directionalShadowVisibility(vec3 normal, vec3 lightDirection) {
    vec4 lightClip = frameData.lightViewProjection * vec4(inWorldPosition, 1.0);
    if (lightClip.w <= 0.0) return 1.0;
    vec3 projected = lightClip.xyz / lightClip.w;
    vec2 uv = projected.xy * 0.5 + 0.5;
    if (projected.z < 0.0 || projected.z > 1.0 ||
        any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)))) return 1.0;

    vec2 texel = 1.0 / vec2(textureSize(directionalShadowMap, 0));
    float bias = max(0.0007, 0.0025 * (1.0 - max(dot(normal, lightDirection), 0.0)));
    float visibility = 0.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            visibility += texture(directionalShadowMap,
                vec3(uv + vec2(float(x), float(y)) * texel, projected.z - bias));
        }
    }
    return visibility / 9.0;
}

vec3 acesToneMap(vec3 color) {
    color = max(color, vec3(0.0));
    return clamp((color * (2.51 * color + 0.03)) /
                 (color * (2.43 * color + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
    vec4 baseSample = (materialData.materialFlags.x & 1u) != 0u
        ? texture(baseColorMap, inUv) : vec4(1.0);
    vec4 base = baseSample * materialData.baseColorFactor;
    if ((materialData.materialFlags.x & 8u) != 0u &&
        base.a < materialData.roughnessNormalOcclusionAlpha.w) discard;

    float metallic = clamp(materialData.emissiveMetallic.w, 0.0, 1.0);
    float roughness = clamp(materialData.roughnessNormalOcclusionAlpha.x, 0.045, 1.0);
    if ((materialData.materialFlags.x & 4u) != 0u) {
        vec4 mr = texture(metallicRoughnessMap, inUv);
        roughness = clamp(roughness * mr.g, 0.045, 1.0);
        metallic = clamp(metallic * mr.b, 0.0, 1.0);
    }

    vec3 n = normalize(inWorldNormal);
    if ((materialData.materialFlags.x & 2u) != 0u) {
        vec3 t = normalize(inWorldTangent.xyz);
        vec3 b = normalize(cross(n, t) * inWorldTangent.w);
        vec3 sampledNormal = texture(normalMap, inUv).xyz * 2.0 - 1.0;
        sampledNormal.xy *= materialData.roughnessNormalOcclusionAlpha.y;
        n = normalize(mat3(t, b, n) * sampledNormal);
    }

    vec3 v = normalize(frameData.cameraPosition.xyz - inWorldPosition);
    vec3 l = normalize(-frameData.directionalLightDirection.xyz);
    vec3 h = normalize(v + l);
    float nDotV = max(dot(n, v), 1e-4);
    float nDotL = max(dot(n, l), 0.0);
    vec3 f0 = mix(vec3(0.04), base.rgb, metallic);
    vec3 f = fresnelSchlick(max(dot(h, v), 0.0), f0);
    float d = distributionGGX(max(dot(n, h), 0.0), roughness);
    float g = geometrySmith(nDotV, nDotL, roughness);
    vec3 specular = (d * g * f) / max(1e-4, 4.0 * nDotV * nDotL);
    vec3 diffuseWeight = (1.0 - f) * (1.0 - metallic);
    vec3 diffuse = diffuseWeight * base.rgb / PI;
    float shadowVisibility = directionalShadowVisibility(n, l);
    vec3 direct = (diffuse + specular) * frameData.directionalLightRadiance.rgb * nDotL * shadowVisibility;

    // Diffuse irradiance is approximated by the broadest environment mip. Specular IBL
    // uses GGX-prefiltered environment mips and a split-sum BRDF integration LUT.
    float environmentMaxLod = float(max(textureQueryLevels(environmentMap) - 1, 0));
    vec3 diffuseIrradiance = textureLod(environmentMap, n, environmentMaxLod).rgb;
    vec3 reflectionDirection = reflect(-v, n);
    vec3 prefilteredEnvironment = textureLod(
        environmentMap, reflectionDirection, roughness * environmentMaxLod).rgb;
    vec3 fresnelRoughness = f0 + (max(vec3(1.0 - roughness), f0) - f0) *
                            pow(1.0 - nDotV, 5.0);
    vec3 diffuseEnvironment = diffuseIrradiance * base.rgb *
                              (1.0 - fresnelRoughness) * (1.0 - metallic);
    vec2 brdf = texture(brdfIntegrationLut, vec2(nDotV, roughness)).rg;
    vec3 specularEnvironment = prefilteredEnvironment * (fresnelRoughness * brdf.x + brdf.y);
    vec3 ambient = (diffuseEnvironment + specularEnvironment) * 0.35;
    vec3 displayLinear = acesToneMap(direct + ambient + materialData.emissiveMetallic.rgb);
    // Current Android swapchain selects UNORM formats, so encode for display here.
    outColor = vec4(pow(displayLinear, vec3(1.0 / 2.2)), base.a);
}
