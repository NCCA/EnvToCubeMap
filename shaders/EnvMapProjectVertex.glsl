#version 410 core
layout (location = 0) in vec3 inVert;

out vec3 localPos;

uniform mat4 VP;

void main()
{
    localPos = inVert;
    gl_Position =  VP * vec4(inVert, 1.0);
}
