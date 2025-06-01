#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 cameraPos;
uniform samplerCube skybox;

void main()
{
    vec3 I = normalize(FragPos - cameraPos);
    vec3 R = reflect(I, normalize(Normal));
    vec3 envColor = texture(skybox, R).rgb;

    // Fresnel efekt: mocniejsze odbicie na brzegach
    float fresnel = pow(1.0 - dot(normalize(Normal), -I), 3.0) * 0.8;

    vec3 color = mix(envColor * 0.2, envColor, fresnel);

    FragColor = vec4(color, 0.4); // pó?przezroczyste szk?o
}
