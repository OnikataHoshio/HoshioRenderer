#version 430 core
out vec4 FragColor;

in vec3 WorldPos; 

void main()
{
    vec3 color = abs(WorldPos);
    color = (color + 0.5) / (color + 0.0001);
    FragColor = vec4(color, 1.0);
}