#version 430 core
layout (location = 0) in vec3 inPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 WorldPos; // 传给像素着色器的变量

void main()
{
    WorldPos = vec3(model * vec4(inPos, 1.0));
    
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}