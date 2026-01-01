#version 330 core
out vec4 FragColor;

in vec2 tex;

uniform sampler2D button_tex;

void main(){
    //FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    FragColor = texture(button_tex, tex);
}