#version 430 core
layout (location = 0) in vec3 inPos;

uniform mat4 view;
uniform mat4 projection;

layout(std430, binding = 0) buffer InstanceBuffer {
    mat4 models[];
};

out vec3 WorldPos; 

void main() {
    mat4 model = models[gl_InstanceID]; 
    WorldPos = vec3(model * vec4(inPos, 1.0));
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}