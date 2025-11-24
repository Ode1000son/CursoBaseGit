#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 3) in mat4 aInstanceModel;

uniform mat4 model;
uniform int uUseInstanceTransform = 0;

void main()
{
    mat4 finalModel = (uUseInstanceTransform == 1) ? aInstanceModel : model;
    gl_Position = finalModel * vec4(aPos, 1.0);
}

