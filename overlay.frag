#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D overlayTex;

void main()
{
    vec4 texColor = texture(overlayTex, TexCoords);
    FragColor = vec4(texColor.rgb, texColor.a * 0.5);
}
