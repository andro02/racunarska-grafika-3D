#version 330 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

out vec3 chFragPos;
out vec3 chNormal;
out vec2 chUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    chUV = inUV * 4.0; 
    chFragPos = vec3(model * vec4(inPos, 1.0));
    chNormal = mat3(transpose(inverse(model))) * inNormal;
    gl_Position = projection * view * vec4(chFragPos, 1.0);
}
