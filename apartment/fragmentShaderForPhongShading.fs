#version 330 core
out vec4 FragColor;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 emissive;
    float shininess;
};

struct DirectionalLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    //float cos_theta;
    float inner_circle;
    float outer_circle;
    
    float k_c;  //attenuation factors
    float k_l;  //attenuation factors
    float k_q;  //attenuation factors
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};


struct PointLight {
    vec3 position;
    
    float k_c;  //attenuation factors
    float k_l;  //attenuation factors
    float k_q;  //attenuation factors
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 emissive;
};

in vec3 FragPos;
in vec3 Normal;

//other variables and instances needed
#define NR_POINT_LIGHTS 2
#define NR_SPOT_LIGHTS 1
uniform bool ambientLight = true;
uniform bool diffuseLight = true;
uniform bool specularLight = true;
uniform bool directionLightOn = true;
uniform bool spotLightOn = false;
uniform vec3 viewPos;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLights[NR_SPOT_LIGHTS];
uniform Material material;
uniform DirectionalLight directionalLight;
uniform SpotLight spotLight;
uniform float alpha = 1.0;
uniform float fogDensity = 0.005; // New fog uniform

//function prototypes
vec3 CalcPointLight(Material material, PointLight light, vec3 N, vec3 fragPos, vec3 V);
vec3 CalcDirectionalLight(Material material, DirectionalLight light, vec3 N, vec3 V);
vec3 CalcSpotLight(Material material, SpotLight light, vec3 N, vec3 fragPos, vec3 V);

void main()
{
    // properties
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    
    vec3 result = vec3(0.0f);

    // point lights
    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(material, pointLights[i], N, FragPos, V);
    
    if(directionLightOn) {
        result += CalcDirectionalLight(material, directionalLight, N, V);
    }
    
    if(spotLightOn){
        result += CalcSpotLight(material, spotLight, N, FragPos, V);
    }

    // --- Fog Calculation ---
    float dist = length(viewPos - FragPos);
    float fogFactor = exp(-pow(max(0.0, dist - 50.0) * fogDensity, 1.5));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    vec3 fogColor = vec3(0.5, 0.8, 0.9); // Deep blue sky haze
    result = mix(fogColor, result, fogFactor);

    FragColor = vec4(result, alpha);
}

// calculates the color when using a point light.
vec3 CalcPointLight(Material material, PointLight light, vec3 N, vec3 fragPos, vec3 V)
{
    vec3 L = normalize(light.position - fragPos);
    vec3 R = reflect(-L, N);
    
    vec3 K_A = material.ambient;
    vec3 K_D = material.diffuse;
    vec3 K_S = material.specular;
    vec3 emissive = material.emissive;
    
    // attenuation
    float d = length(light.position - fragPos);
    float attenuation = 1.0 / (light.k_c + light.k_l * d + light.k_q * (d * d));
    
    vec3 ambient = K_A * light.ambient;
    vec3 diffuse = K_D * max(dot(N, L), 0.0) * light.diffuse;
    vec3 specular = K_S * pow(max(dot(V, R), 0.0), material.shininess) * light.specular;
    
    // respect global component toggles
    if(!ambientLight) ambient = vec3(0.0);
    if(!diffuseLight) diffuse = vec3(0.0);
    if(!specularLight) specular = vec3(0.0);
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    return (ambient + diffuse + specular + emissive);
}

vec3 CalcDirectionalLight(Material material, DirectionalLight light, vec3 N, vec3 V)
{
    vec3 L = normalize(-light.direction);
    vec3 R = reflect(-L, N);
    
    vec3 K_A = material.ambient;
    vec3 K_D = material.diffuse;
    vec3 K_S = material.specular;
    
    vec3 ambient, diffuse, specular;

    if(ambientLight){
        ambient = K_A * light.ambient;
    }
    else{
        ambient = vec3(0.0f, 0.0f, 0.0f);
    }

    if(diffuseLight){
        diffuse = K_D * max(dot(N, L), 0.0) * light.diffuse;
    }
    else{
        diffuse = vec3(0.0f, 0.0f, 0.0f);
    }

    if(specularLight){
        specular = K_S * pow(max(dot(V, R), 0.0), material.shininess) * light.specular;
    }
    else{
        specular = vec3(0.0f, 0.0f, 0.0f);
    }
    
    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(Material material, SpotLight light, vec3 N, vec3 fragPos, vec3 V)
{
    vec3 L = normalize(light.position - fragPos);
    vec3 R = reflect(-L, N);
    
    vec3 K_A = material.ambient;
    vec3 K_D = material.diffuse;
    vec3 K_S = material.specular;
    
    // attenuation
    float d = length(light.position - fragPos);
    float attenuation = 1.0 / (light.k_c + light.k_l * d + light.k_q * (d * d));

    float cos_alpha = dot(L, normalize(-light.direction));
    float cos_theta = light.inner_circle- light.outer_circle;
    
    float intensity = clamp((cos_alpha-light.outer_circle)/cos_theta, 0.0, 1.0);   
    
    vec3 ambient = K_A * light.ambient;
    vec3 diffuse = K_D * max(dot(N, L), 0.0) * light.diffuse;
    vec3 specular = K_S * pow(max(dot(V, R), 0.0), material.shininess) * light.specular;

    // respect global component toggles
    if(!ambientLight) ambient = vec3(0.0);
    if(!diffuseLight) diffuse = vec3(0.0);
    if(!specularLight) specular = vec3(0.0);

    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    return (ambient + diffuse + specular);
}
