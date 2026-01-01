#version 330 core
out vec4 FragColor;

in VS_OUT{
    vec3 frag_pos;
    vec2 tex_coord;
    vec3 normal;
    vec3 light_pos;
} fs_in;

struct material{
    vec3 kd;
    bool has_kd_map;
    sampler2D diffuse_map;

    vec3 ks;
    bool has_ks_map;
    sampler2D spec_map;

    float ns;

    bool has_alpha_value;

    float d;
};

uniform material mat;
uniform vec3 view_pos;
uniform bool first_pass;
uniform sampler2D prev_depth;
uniform vec2 screen_size;

void main(){
    if(!first_pass){
        if(gl_FragCoord.z <= texture(prev_depth,( gl_FragCoord.xy / screen_size)).r){
            discard;
        }
    }

    float alpha;

    if(mat.has_alpha_value){
        alpha = texture(mat.diffuse_map, fs_in.tex_coord).a;
        
    }
    else{
        alpha = mat.d;
    }

    if(alpha < 0.1){
            discard;
        }

    vec3 color;
    if(!mat.has_kd_map){
        color = mat.kd;
        color = pow(color, vec3(2.2));
    }
    else{
        color = vec3(texture(mat.diffuse_map, fs_in.tex_coord));
    }

    vec3 spec_color;
    if(!mat.has_ks_map){
        spec_color = mat.ks;
    }
    else{
        spec_color = vec3(texture(mat.spec_map, fs_in.tex_coord));
    }

    vec3 ambient = 0.05 * color;

    vec3 light_dir = normalize(fs_in.light_pos - fs_in.frag_pos);
    vec3 normal = normalize(fs_in.normal);
    float diff = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = diff * color;

    vec3 view_dir = normalize(-fs_in.frag_pos);
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float spec = pow(max(dot(normal, halfway_dir), 0.0), mat.ns);
    vec3 specular = spec_color * spec;


    vec3 color_final = (ambient + diffuse + specular);
    FragColor = vec4(color_final * alpha, alpha);
}