#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 waterColor = vec3(0.1, 0.3, 0.6);
uniform samplerCube skybox;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 I = normalize(FragPos - viewPos);
    vec3 R = reflect(I, norm);
    vec3 reflectionColor = texture(skybox, R).rgb;
    vec3 lightDir = normalize(lightPos);
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    float specularStrength = 1.0;
    vec3 viewDir = -I;
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 128);
    vec3 specular = specularStrength * spec * lightColor;
    float fresnel = 0.05 + 0.95 * pow(1.0 - max(dot(norm, viewDir), 0.0), 5.0);
    vec3 baseColor = (ambient + diffuse) * waterColor;
    vec3 finalColor = mix(baseColor, reflectionColor, fresnel);
    FragColor = vec4(finalColor + specular, 0.80); //TUTAJ JEST ZMIANA PRZEZROCZYSTOSCI
}