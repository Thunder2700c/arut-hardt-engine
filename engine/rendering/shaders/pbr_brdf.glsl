#ifndef ARUT_PBR_BRDF_GLSL
#define ARUT_PBR_BRDF_GLSL

const float ARUT_PBR_PI = 3.14159265358979323846;
float arutPbrSaturate(float value) { return clamp(value, 0.0, 1.0); }
vec3 arutPbrFresnelSchlick(float cosTheta, vec3 f0) {
    return f0 + (1.0 - f0) * pow(1.0 - arutPbrSaturate(cosTheta), 5.0);
}
float arutPbrDistributionGGX(float nDotH, float roughness) {
    float alpha = max(0.045, arutPbrSaturate(roughness));
    float alpha2 = alpha * alpha * alpha * alpha;
    float n = arutPbrSaturate(nDotH);
    float denominator = n * n * (alpha2 - 1.0) + 1.0;
    return alpha2 / max(1.0e-7, ARUT_PBR_PI * denominator * denominator);
}
float arutPbrGeometrySchlickGGX(float nDot, float roughness) {
    float r = arutPbrSaturate(roughness) + 1.0;
    float k = r * r * 0.125;
    float n = arutPbrSaturate(nDot);
    return n / max(1.0e-7, n * (1.0 - k) + k);
}
float arutPbrGeometrySmith(float nDotV, float nDotL, float roughness) {
    return arutPbrGeometrySchlickGGX(nDotV, roughness) *
           arutPbrGeometrySchlickGGX(nDotL, roughness);
}
vec3 arutPbrEvaluateDirect(vec3 normal, vec3 viewDirection, vec3 lightDirection,
                           vec3 baseColor, float metallic, float roughness, vec3 radiance) {
    vec3 n = normalize(normal);
    vec3 v = normalize(viewDirection);
    vec3 l = normalize(lightDirection);
    vec3 h = normalize(v + l);
    float nDotV = max(1.0e-4, dot(n, v));
    float nDotL = max(0.0, dot(n, l));
    if (nDotL <= 0.0) return vec3(0.0);
    float nDotH = max(0.0, dot(n, h));
    float hDotV = max(0.0, dot(h, v));
    float m = arutPbrSaturate(metallic);
    float r = clamp(arutPbrSaturate(roughness), 0.045, 1.0);
    vec3 f0 = mix(vec3(0.04), clamp(baseColor, 0.0, 1.0), m);
    vec3 f = arutPbrFresnelSchlick(hDotV, f0);
    float d = arutPbrDistributionGGX(nDotH, r);
    float g = arutPbrGeometrySmith(nDotV, nDotL, r);
    vec3 specular = f * (d * g / max(1.0e-4, 4.0 * nDotV * nDotL));
    vec3 diffuseWeight = (vec3(1.0) - f) * (1.0 - m);
    vec3 diffuse = diffuseWeight * clamp(baseColor, 0.0, 1.0) * (1.0 / ARUT_PBR_PI);
    return (diffuse + specular) * radiance * nDotL;
}
#endif
