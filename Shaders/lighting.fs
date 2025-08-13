#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

struct Light {
    vec3 direction;
    vec3 color;
};

uniform Light sunLight;
uniform vec3 staffLight.position;
uniform vec3 staffLight.color;
uniform vec3 fireballLight.position;
uniform vec3 fireballLight.color;

uniform sampler2D texture_diffuse;
uniform sampler2D shadowMap;
uniform vec3 viewPos;
uniform float alpha = 1.0;
uniform vec3 overrideColor = vec3(-1.0); // Use -1 as a flag for no override

float calculateShadow(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLightSpace as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float shadow = 0.0;
    float bias = 0.005; 
    if(currentDepth - bias > closestDepth)
        shadow = 1.0;
        
    return shadow;
}

vec3 calculatePointLight(vec3 lightPos, vec3 lightColor, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(lightPos - fragPos);
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = 0.5 * spec * lightColor;

    // Attenuation
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.02 * (distance * distance));
    
    return (diffuse + specular) * attenuation * baseColor;
}

void main()
{    
    vec3 color = texture(texture_diffuse, TexCoords).rgb;
    if (overrideColor.r >= 0.0) {
        color = overrideColor;
    }
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // Sun Light (Directional)
    float shadow = calculateShadow(FragPosLightSpace);
    vec3 lightDir = normalize(-sunLight.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * sunLight.color;
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = 0.5 * spec * sunLight.color;
    vec3 sunContribution = (diffuse + specular) * (1.0 - shadow);

    // Total lighting
    vec3 lighting = (0.2 * color) + sunContribution * color; // Ambient + Sun

    // Staff Light (Point)
    if (staffLight.position.y > -999.0) {
        lighting += calculatePointLight(staffLight.position, staffLight.color, normal, FragPos, viewDir, color);
    }
    
    // Fireball Light (Point)
    if (fireballLight.position.y > -999.0) {
        lighting += calculatePointLight(fireballLight.position, fireballLight.color, normal, FragPos, viewDir, color);
    }

    // Fog
    float distanceToCam = length(viewPos - FragPos);
    float fogFactor = exp(-0.02 * distanceToCam);
    fogFactor = clamp(fogFactor, 0.1, 1.0);
    vec3 fogColor = vec3(0.1, 0.2, 0.3);

    FragColor = vec4(mix(fogColor, lighting, fogFactor), alpha);
}