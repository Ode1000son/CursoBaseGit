#version 330 core

// Fragment shader para shadow mapping de luz pontual
// Calcula distância linearizada da luz e armazena no depth buffer
// Usado para gerar cubemap de profundidade para point light shadows

in vec4 FragPos;  // Posição do fragmento no espaço da luz

uniform vec3 lightPos;    // Posição da luz pontual
uniform float far_plane;   // Plano distante do frustum da luz

void main()
{
    // Calcula distância linearizada da luz ao fragmento
    float lightDistance = length(FragPos.xyz - lightPos);
    lightDistance = lightDistance / far_plane;  // Normaliza para [0, 1]
    gl_FragDepth = lightDistance;  // Armazena no depth buffer
}

