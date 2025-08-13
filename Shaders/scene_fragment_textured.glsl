#version 330 core

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 Tex;
    vec4 FragPosLightSpace;
} fs_in;

// ---------- textures ----------
uniform sampler2D texture_diffuse1;  // surface base color
uniform sampler2D shadowMap;         // sun shadow depth

// ---------- camera ----------
uniform vec3 viewPos;

// ---------- sun (directional-ish) ----------
uniform vec3 lightPos;   // you animate this around the origin
uniform vec3 lightColor; // e.g., white-ish

// ---------- snake hit flash ----------
uniform float hitFlashStrength; // 0..1
uniform vec3  hitFlashColor;    // usually red

// ---------- fireball point lights ----------
#define MAX_FIREBALLS 32
uniform int  uFireballCount;
uniform vec3 uFireballPos[MAX_FIREBALLS];
uniform vec3 uFireballColor[MAX_FIREBALLS];
uniform float uFireballRadius; // falloff distance scale (try 6..10)

// ---------- fireball emissive toggle on the sphere itself ----------
uniform float isFireball; // 1.0 when drawing the fireball mesh

out vec4 FragColor;

// ------- shadow helper (PCF) -------
float ShadowFactor(vec4 fragPosLightSpace, vec3 N, vec3 L)
{
    // perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // to [0,1]
    projCoords = projCoords * 0.5 + 0.5;

    // outside light frustum? no shadowing
    if (projCoords.z > 1.0) return 0.0;

    float bias = max(0.0005 * (1.0 - dot(N, L)), 0.0002);
    float currentDepth = projCoords.z;

    // 3x3 PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x=-1; x<=1; ++x)
    for(int y=-1; y<=1; ++y)
    {
        float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x,y) * texelSize).r;
        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
    }
    shadow /= 9.0;
    return shadow;
}

// ------- simple point light with quadratic falloff -------
vec3 PointLight(vec3 lp, vec3 lc, vec3 fragPos, vec3 N)
{
    vec3 Lvec = lp - fragPos;
    float d   = length(Lvec);
    vec3 L    = Lvec / max(d, 1e-6);

    float ndotl = max(dot(N, L), 0.0);

    // Smooth quadratic attenuation: intensity ~ 1 / (1 + (d/r)^2)
    // replace your attenuation line inside PointLight():
    float r = max(uFireballRadius, 1e-3);
    float att = 1.0 / (1.0 + pow(d / r, 4.0));  // much tighter than quadratic

    return lc * ndotl * att;
}

void main()
{
    vec3 base = texture(texture_diffuse1, fs_in.Tex).rgb;

    // world-space normal
    vec3 N = normalize(fs_in.Normal);

    // ---------- SUN (treated as directional) ----------
    // We approximate sun direction from lightPos toward scene origin.
    vec3 sunDir = normalize(-lightPos); // points from surface toward sun
    float sunDiff = max(dot(N, sunDir), 0.0);

    // shadows for sun only
    float shadow = ShadowFactor(fs_in.FragPosLightSpace, N, sunDir);
    vec3 sunLight = lightColor * sunDiff * (1.0 - shadow);

    // ---------- FIREBALL POINT LIGHTS ----------
    vec3 fireballLight = vec3(0.0);
    for (int i = 0; i < uFireballCount; ++i) {
        fireballLight += PointLight(uFireballPos[i], uFireballColor[i], fs_in.FragPos, N);
    }

    // ---------- HIT FLASH overlay ----------
    vec3 hitFlash = mix(vec3(0.0), hitFlashColor, clamp(hitFlashStrength, 0.0, 1.0));

    // ---------- emissive for the fireball mesh itself ----------
    vec3 emissive = (isFireball > 0.5)
        ? vec3(1.0, 0.45, 0.15) * 2.0    // bright orange core
        : vec3(0.0);

    // ---------- assemble ----------
    vec3 lighting = sunLight + fireballLight;
    vec3 color = base * lighting + emissive + hitFlash;

    FragColor = vec4(color, 1.0);
}
