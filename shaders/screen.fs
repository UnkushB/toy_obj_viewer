#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec2 screen_size;

void main()
{
    vec4 back = texture(screenTexture, TexCoords);
    FragColor = vec4(pow(back.rgb, vec3(1.0/2.2)), back.a);

} 