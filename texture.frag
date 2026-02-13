#version 330 core
out vec4 FragColor;

in vec3 chFragPos;
in vec3 chNormal;
in vec2 chUV;

uniform sampler2D uTex;
uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform vec3 uLightColor;

// --- Treasure light ---
uniform bool uTreasureLightEnabled;
uniform vec3 uGemLightPos;
uniform vec3 uGemLightColor;
uniform float uGemLightIntensity;

uniform vec3 uCoin1LightPos;
uniform vec3 uCoin2LightPos;
uniform vec3 uCoinLightColor;
uniform float uCoinLightIntensity;

void main()
{
    // Boja iz teksture
    vec3 color = texture(uTex, chUV).rgb;

    // Normala
    vec3 norm = normalize(chNormal);

    // --- Glavno svetlo ---
    vec3 lightDir = normalize(uLightPos - chFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;
    vec3 ambient = 0.2 * color;
    vec3 viewDir = normalize(uViewPos - chFragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = 0.5 * spec * uLightColor;

    vec3 result = ambient + (diffuse + specular) * color;

    // --- Treasure light (samo ako je enabled) ---
    if(uTreasureLightEnabled)
    {
        vec3 tLightDir = normalize(uGemLightPos - chFragPos);

        // Podrška za backface osvetljenje
        float tDiff = max(dot(norm, tLightDir), 0.0);
        float tDiffuseFactor = max(dot(norm, tLightDir), 0.0);

        // Kvadratna attenuation
        float distance = length(uGemLightPos - chFragPos);
        float attenuation = 1.0 / (distance * distance);

        vec3 tDiffuse = tDiffuseFactor * uGemLightColor * uGemLightIntensity * attenuation;

        // Specular komponenta Gem light-a
        vec3 tReflect = reflect(-tLightDir, norm);
        float tSpec = pow(max(dot(viewDir, tReflect), 0.0), 32);
        vec3 tSpecular = 0.3 * tSpec * uGemLightColor * uGemLightIntensity * attenuation;

        result += tDiffuse + tSpecular;

        tLightDir = normalize(uCoin1LightPos - chFragPos);

        // Podrška za backface osvetljenje
        tDiff = max(dot(norm, tLightDir), 0.0);
        tDiffuseFactor = max(dot(norm, tLightDir), 0.0);

        // Kvadratna attenuation
        distance = length(uCoin1LightPos - chFragPos);
        attenuation = 1.0 / (distance * distance);

        tDiffuse = tDiffuseFactor * uCoinLightColor * uCoinLightIntensity * attenuation;

        // Specular komponenta Coin1 light-a
        tReflect = reflect(-tLightDir, norm);
        tSpec = pow(max(dot(viewDir, tReflect), 0.0), 32);
        tSpecular = 0.3 * tSpec * uCoinLightColor * uCoinLightIntensity * attenuation;

        result += tDiffuse + tSpecular;

        tLightDir = normalize(uCoin2LightPos - chFragPos);

        // Podrška za backface osvetljenje
        tDiff = max(dot(norm, tLightDir), 0.0);
        tDiffuseFactor = max(dot(norm, tLightDir), 0.0);

        // Kvadratna attenuation
        distance = length(uCoin2LightPos - chFragPos);
        attenuation = 1.0 / (distance * distance);

        tDiffuse = tDiffuseFactor * uCoinLightColor * uCoinLightIntensity * attenuation;

        // Specular komponenta Coin2 light-a
        tReflect = reflect(-tLightDir, norm);
        tSpec = pow(max(dot(viewDir, tReflect), 0.0), 32);
        tSpecular = 0.3 * tSpec * uCoinLightColor * uCoinLightIntensity * attenuation;

        result += tDiffuse + tSpecular;
    }

    FragColor = vec4(result, 1.0);
}
