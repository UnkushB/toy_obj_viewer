#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 2) in vec3 norm;

out VS_OUT{
    vec3 frag_pos;
    vec2 tex_coord;
    vec3 normal;
    vec3 light_pos;
} vs_out;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;
uniform vec3 light_location;
uniform mat4 lightSpaceMatrix;

void main(){
    vec4 frag_model_space = model * vec4(pos, 1.0);
    vs_out.frag_pos = vec3(view * frag_model_space);
    vs_out.tex_coord = tex;
    vs_out.normal = mat3(transpose(inverse(view * model))) * norm;
    vs_out.light_pos = vec3(view *  vec4(light_location, 1.0));
    gl_Position = projection * view * model * vec4(pos, 1.0);
}