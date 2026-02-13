#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D uDiffMap; 
uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform vec3 uLightColor;

void main()
{
    vec3 color = texture(uDiffMap, TexCoords).rgb;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPos - FragPos);

    // Ambient
    vec3 ambient = 0.2 * color;

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * color * uLightColor;

    // Specular
    vec3 viewDir = normalize(uViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = 0.5 * spec * uLightColor;

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
