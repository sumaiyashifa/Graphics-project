#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 VertexColor;

// ── Texture mode ──────────────────────────────────────────────────────────────
// 0 = No texture  (solid material color)
// 1 = Simple texture (pure texture, no surface color blend)
// 2 = Vertex-blended texture  (texture * vertex color)
// 3 = Fragment-blended texture (texture * material diffuse computed per fragment)
uniform int textureMode;
uniform sampler2D texture1;

// ── Material ──────────────────────────────────────────────────────────────────
struct Material {
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float shininess;
    vec3  emissive;
};
uniform Material material;

// ── Directional light ─────────────────────────────────────────────────────────
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight directionalLight;
uniform bool     directionLightOn;

// ── Point lights (max 2) ──────────────────────────────────────────────────────
struct PointLight {
    vec3  position;
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float k_c, k_l, k_q;
};
uniform PointLight pointLights[2];

// ── Spot light ────────────────────────────────────────────────────────────────
struct SpotLight {
    vec3  position;
    vec3  direction;
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float k_c, k_l, k_q;
    float inner_circle;
    float outer_circle;
};
uniform SpotLight spotLight;
uniform bool      spotLightOn;

// ── Component toggles ─────────────────────────────────────────────────────────
uniform bool ambientLight;
uniform bool diffuseLight;
uniform bool specularLight;

uniform vec3 viewPos;

// ─────────────────────────────────────────────────────────────────────────────
// Helper: resolve the base colour depending on textureMode
// ─────────────────────────────────────────────────────────────────────────────
vec3 resolveBaseColor()
{
    if (textureMode == 1) {
        // Simple texture – ignore material diffuse entirely
        return texture(texture1, TexCoord).rgb;
    }
    else if (textureMode == 2) {
        // Vertex-blended: texture * colour passed through vertex shader
        return texture(texture1, TexCoord).rgb * VertexColor;
    }
    else if (textureMode == 3) {
        // Fragment-blended: texture * material.diffuse (computed per fragment)
        return texture(texture1, TexCoord).rgb * material.diffuse;
    }
    // textureMode == 0 : plain material
    return material.diffuse;
}

// ─────────────────────────────────────────────────────────────────────────────
// Directional light contribution
// ─────────────────────────────────────────────────────────────────────────────
vec3 calcDirLight(vec3 norm, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(-directionalLight.direction);
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3  reflDir = reflect(-lightDir, norm);
    float spec    = pow(max(dot(viewDir, reflDir), 0.0), material.shininess);

    vec3 ambient  = ambientLight  ? directionalLight.ambient  * (textureMode != 0 ? baseColor * 0.4 : material.ambient) : vec3(0.0);
    vec3 diffuse  = diffuseLight  ? directionalLight.diffuse  * diff * baseColor : vec3(0.0);
    vec3 specular = specularLight ? directionalLight.specular * spec * material.specular : vec3(0.0);

    return ambient + diffuse + specular;
}

// ─────────────────────────────────────────────────────────────────────────────
// Point light contribution
// ─────────────────────────────────────────────────────────────────────────────
vec3 calcPointLight(PointLight light, vec3 norm, vec3 viewDir, vec3 baseColor)
{
    vec3  lightDir = normalize(light.position - FragPos);
    float diff     = max(dot(norm, lightDir), 0.0);
    vec3  reflDir  = reflect(-lightDir, norm);
    float spec     = pow(max(dot(viewDir, reflDir), 0.0), material.shininess);

    float dist  = length(light.position - FragPos);
    float atten = 1.0 / (light.k_c + light.k_l * dist + light.k_q * dist * dist);

    vec3 ambient  = ambientLight  ? light.ambient  * (textureMode != 0 ? baseColor * 0.4 : material.ambient) : vec3(0.0);
    vec3 diffuse  = diffuseLight  ? light.diffuse  * diff * baseColor : vec3(0.0);
    vec3 specular = specularLight ? light.specular * spec * material.specular : vec3(0.0);

    return (ambient + diffuse + specular) * atten;
}

// ─────────────────────────────────────────────────────────────────────────────
// Spot light contribution
// ─────────────────────────────────────────────────────────────────────────────
vec3 calcSpotLight(vec3 norm, vec3 viewDir, vec3 baseColor)
{
    vec3  lightDir = normalize(spotLight.position - FragPos);
    float theta    = dot(lightDir, normalize(-spotLight.direction));
    float epsilon  = spotLight.inner_circle - spotLight.outer_circle;
    float intensity = clamp((theta - spotLight.outer_circle) / epsilon, 0.0, 1.0);

    float diff    = max(dot(norm, lightDir), 0.0);
    vec3  reflDir = reflect(-lightDir, norm);
    float spec    = pow(max(dot(viewDir, reflDir), 0.0), material.shininess);

    float dist  = length(spotLight.position - FragPos);
    float atten = 1.0 / (spotLight.k_c + spotLight.k_l * dist + spotLight.k_q * dist * dist);

    vec3 ambient  = ambientLight  ? spotLight.ambient  * (textureMode != 0 ? baseColor * 0.4 : material.ambient) : vec3(0.0);
    vec3 diffuse  = diffuseLight  ? spotLight.diffuse  * diff * baseColor : vec3(0.0);
    vec3 specular = specularLight ? spotLight.specular * spec * material.specular : vec3(0.0);

    return (ambient + diffuse * intensity + specular * intensity) * atten;
}

uniform float alpha = 1.0;
uniform float fogDensity = 0.005; // New fog uniform

// ─────────────────────────────────────────────────────────────────────────────
void main()
{
    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 baseColor = resolveBaseColor();

    vec3 result = material.emissive;

    if (directionLightOn)
        result += calcDirLight(norm, viewDir, baseColor);

    // Point lights
    result += calcPointLight(pointLights[0], norm, viewDir, baseColor);
    result += calcPointLight(pointLights[1], norm, viewDir, baseColor);

    if (spotLightOn)
        result += calcSpotLight(norm, viewDir, baseColor);

    // --- Fog Calculation ---
    float dist = length(viewPos - FragPos);
    float fogFactor = exp(-pow(max(0.0, dist - 50.0) * fogDensity, 1.5));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    vec3 fogColor = vec3(0.5, 0.8, 0.9); // Deep blue sky haze
    result = mix(fogColor, result, fogFactor);

    FragColor = vec4(result, alpha);
}
