#version 330 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(inPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * inNormal;
    TexCoords = inUV;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
