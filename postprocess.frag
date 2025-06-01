#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{

    float intensity = 1.5; 

    vec4 sceneSample = texture(screenTexture, TexCoords); 
    vec3 color = sceneSample.rgb; 
    
    float grayscaleVal = dot(color, vec3(0.2126, 0.7152, 0.0722));
    vec3 grayEquivalent = vec3(grayscaleVal);
    
    vec3 intenseColor = mix(grayEquivalent, color, intensity);
    
    FragColor = vec4(intenseColor, sceneSample.a); 
}