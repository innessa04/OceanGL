#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float time;

vec3 GerstnerWave(vec3 pos, float steepness, float wavelength, vec2 direction, float speed) {
    float k = 2.0 * 3.14159 / wavelength;
    float c = sqrt(9.8 / k) * speed;
    vec2 d = normalize(direction);
    float f = k * (dot(d, pos.xz) - c * time);
    float a = steepness / k;

    pos.x += d.x * a * cos(f);
    pos.y += a * sin(f);
    pos.z += d.y * a * cos(f);

    return pos;
}

vec3 CalculateNormal(vec3 pos) {
    float H = 0.01;
    vec3 p_x = vec3(pos.x + H, 0.0, pos.z);
    vec3 p_z = vec3(pos.x, 0.0, pos.z + H);

    vec3 current_p = pos; //Start with original pos for normal calc base

    //Apply the same waves as in main
    current_p = GerstnerWave(current_p, 0.2, 15.0, vec2(1.0, 0.5), 1.0);
    current_p = GerstnerWave(current_p, 0.1, 5.0, vec2(-0.3, 0.8), 1.5);
    current_p = GerstnerWave(current_p, 0.05, 2.0, vec2(0.5, -0.5), 0.8);

    p_x = GerstnerWave(p_x, 0.2, 15.0, vec2(1.0, 0.5), 1.0);
    p_x = GerstnerWave(p_x, 0.1, 5.0, vec2(-0.3, 0.8), 1.5);
    p_x = GerstnerWave(p_x, 0.05, 2.0, vec2(0.5, -0.5), 0.8);

    p_z = GerstnerWave(p_z, 0.2, 15.0, vec2(1.0, 0.5), 1.0);
    p_z = GerstnerWave(p_z, 0.1, 5.0, vec2(-0.3, 0.8), 1.5);
    p_z = GerstnerWave(p_z, 0.05, 2.0, vec2(0.5, -0.5), 0.8);

    vec3 tangent = normalize(p_x - current_p);
    vec3 bitangent = normalize(p_z - current_p);

    return normalize(cross(bitangent, tangent));
}


void main()
{
    vec3 wavedPos = aPos;

    wavedPos = GerstnerWave(wavedPos, 0.2, 15.0, vec2(1.0, 0.5), 1.0);
    wavedPos = GerstnerWave(wavedPos, 0.1, 5.0, vec2(-0.3, 0.8), 1.5);
    wavedPos = GerstnerWave(wavedPos, 0.05, 2.0, vec2(0.5, -0.5), 0.8);

    FragPos = vec3(model * vec4(wavedPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * CalculateNormal(aPos);

    gl_Position = projection * view * vec4(FragPos, 1.0);
}