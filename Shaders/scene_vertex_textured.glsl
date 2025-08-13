#version 330 core

layout (location = 0) in vec3 aPos;      // position (world model space)
layout (location = 1) in vec3 aNormal;   // normal  (model space)
layout (location = 2) in vec2 aTex;      // uv

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix; // for shadowing from the sun

out VS_OUT {
    vec3 FragPos;               // world-space position
    vec3 Normal;                // world-space normal
    vec2 Tex;
    vec4 FragPosLightSpace;     // for shadow map lookup
} vs_out;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    vs_out.FragPos = worldPos.xyz;

    // correct normal transform (assumes no non-uniform scale; if you have, add normalMatrix)
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vs_out.Normal = normalize(normalMatrix * aNormal);

    vs_out.Tex = aTex;
    vs_out.FragPosLightSpace = lightSpaceMatrix * worldPos;

    gl_Position = projection * view * worldPos;
}
