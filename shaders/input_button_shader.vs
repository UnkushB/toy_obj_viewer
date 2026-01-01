#version 330 core
layout (location = 0) in vec3 coord;
layout (location = 1) in vec2 tex_coord;

out vec2 tex;

void main(){
    gl_Position = vec4(coord, 1.0);
    tex = tex_coord;
}