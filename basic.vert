#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out vec3 chNormal;
out vec3 chFragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    chFragPos = vec3(model * vec4(aPos, 1.0));
    chNormal = mat3(transpose(inverse(model))) * aNormal;

    gl_Position = projection * view * vec4(chFragPos, 1.0);
}
