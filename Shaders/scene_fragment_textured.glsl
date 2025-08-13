#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

uniform sampler2D texture_diffuse1;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

// Hit flash effect
uniform vec3 hitFlashColor = vec3(1.0, 0.0, 0.0);
uniform float hitFlashStrength = 0.0;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    
    float bias = 0.005;
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    
    return shadow;
}

void main()
{
    // DEBUG: Simple solid color to test if geometry is working
    // Uncomment this line to see if models render as solid red
    // FragColor = vec4(1.0, 0.0, 0.0, 1.0); return;
    
    vec3 color = texture(texture_diffuse1, TexCoord).rgb;
    vec3 normal = normalize(Normal);
    vec3 lightColorLocal = vec3(1.0);
    
    // Ambient
    vec3 ambient = 0.15 * color;
    
    // Diffuse
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColorLocal;
    
    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColorLocal;
    
    // Calculate shadow
    float shadow = ShadowCalculation(FragPosLightSpace);
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;
    
    // Apply hit flash effect
    if (hitFlashStrength > 0.0) {
        lighting = mix(lighting, hitFlashColor, hitFlashStrength);
    }
    
    FragColor = vec4(lighting, 1.0);
}