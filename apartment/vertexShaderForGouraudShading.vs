#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec4 LightingColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

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

struct PointLight {
    vec3 position;
    
    float k_c;      //attenuation factors
    float k_l;      //attenuation factors
    float k_q;      //attenuation factors
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 emissive;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    
    float cos_theta;
    
    float k_c;      //attenuation factors
    float k_l;      //attenuation factors
    float k_q;      //attenuation factors
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

//other variables and instances needed
#define NR_POINT_LIGHTS 2
uniform bool ambientLight = true;
uniform bool diffuseLight = true;
uniform bool specularLight = true;
uniform bool directionLightOn = true;
uniform bool spotLightOn = false;
uniform vec3 viewPos;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform Material material;
uniform DirectionalLight directionalLight;
uniform SpotLight spotLight;

//function prototypes
vec3 CalcPointLight(Material material, PointLight light, vec3 N, vec3 Pos, vec3 V);
vec3 CalcDirectionalLight(Material material, DirectionalLight light, vec3 N, vec3 V);
vec3 CalcSpotLight(Material material, SpotLight light, vec3 N, vec3 Pos, vec3 V);

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    
    vec3 Pos = vec3(model * vec4(aPos, 1.0));
    vec3 Normal = mat3(transpose(inverse(model))) * aNormal;
    
    //properties
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - Pos);

    vec3 result = vec3(0.0f);
    
    //lights
    for(int i = 0; i < NR_POINT_LIGHTS; i++){
        result += CalcPointLight(material, pointLights[i], N, Pos, V);
    }
    if(directionLightOn){
        result += CalcDirectionalLight(material, directionalLight, N, V);
    }
    if(spotLightOn){
        result += CalcSpotLight(material, spotLight, N, Pos, V);
    }
    LightingColor = vec4(result, 1.0);    
}

//calculates the color when using a point light
vec3 CalcPointLight(Material material, PointLight light, vec3 N, vec3 Pos, vec3 V)
{
    vec3 L = normalize(light.position - Pos);
    vec3 R = reflect(-L, N);
    
    vec3 K_A = material.ambient;
    vec3 K_D = material.diffuse;
    vec3 K_S = material.specular;
    vec3 emissive = material.emissive;
    
    // attenuation
    float d = length(light.position - Pos);
    float attenuation = 1/(light.k_c + light.k_l * d + light.k_q * (d * d));
    
    vec3 ambient = K_A * light.ambient;
    vec3 diffuse = K_D * max(dot(N, L), 0.0) * light.diffuse;
    vec3 specular = K_S * pow(max(dot(V, R), 0.0), material.shininess) * light.specular;
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    return (ambient + diffuse + specular + emissive);
}

//calculates the color when using directional light
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

//calculates the color when using spot light
vec3 CalcSpotLight(Material material, SpotLight light, vec3 N, vec3 Pos, vec3 V)
{
    vec3 L = normalize(light.position - Pos);
    vec3 R = reflect(-L, N);
    
    vec3 K_A = material.ambient;
    vec3 K_D = material.diffuse;
    vec3 K_S = material.specular;
    
    // attenuation
    float d = length(light.position - Pos);
    float attenuation = 1/(light.k_c + light.k_l * d + light.k_q * (d * d));
    
    vec3 ambient = K_A * light.ambient;
    vec3 diffuse = K_D * max(dot(N, L), 0.0) * light.diffuse;
    vec3 specular = K_S * pow(max(dot(V, R), 0.001), material.shininess) * light.specular;
    
    float cos_alpha = dot(L, normalize(-light.direction));
    float intensity;
    if(cos_alpha < light.cos_theta){
        intensity = 0.0f;
    }
    else{
        intensity = cos_alpha;
    }

    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    return (ambient + diffuse + specular);
}