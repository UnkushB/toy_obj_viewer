#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <glm/glm.hpp>
#include <algorithm>
#include <sstream>
#include <filesystem>
#include <unordered_map>

bool is_white_space(char c);
void skip_white_space_forward(std::string& line, int& index);
float get_x(std::string& line, int index);
glm::vec2 get_xy(std::string& line, int index);
glm::vec3 get_xyz(std::string& line, int index);
std::string get_line_type(std::string& line, int& index);
std::string get_current_directory(const char* model_file_path);

glm::vec3 sumAC(0), true_centroid(0);
float total_area = 0;

enum face_type {
    v_vt_vn,
    v_vn,
    unsupported
};

face_type supported_face_type(std::string& line, int temp_index);

class mat {
public:
    glm::vec3 kd, ks;
    float ns, d;
    bool has_kd_map, has_ks_map;
    unsigned char* kd_data, * ks_data;
    int kd_width, kd_height, ks_width, ks_height, kd_nr_channels, ks_nr_channels;

    mat() : kd(0.2), ks(1.0), ns{ 32.0 }, d{ 1.0 }, has_kd_map{ false }, has_ks_map{ false }, kd_data(nullptr),
        ks_data(nullptr),
        kd_width(0), kd_height(0),
        ks_width(0), ks_height(0),
        kd_nr_channels(0), ks_nr_channels(0) {
    }
};

std::unordered_map<std::string, mat> materials;

class vertex {
public:
    glm::vec3 vertexCord;
    glm::vec2 textureCord;
    glm::vec3 vertexNormal;

    vertex(const glm::vec3& vertexCord, const glm::vec2& textureCord, const glm::vec3& vertexNormal) :
        vertexCord(vertexCord), textureCord(textureCord), vertexNormal(vertexNormal) {
    }
};

class mesh {
public:
    std::vector<vertex> mesh_vertices;
    glm::vec3 mesh_centroid;
    float mesh_area;
    glm::vec3 meshAC;
    mat* mesh_mat;
    unsigned int vao, vbo, diffuse_map, spec_map;
    bool has_alpha_val;

    mesh() :mesh_area{ 0 }, meshAC(0.0f), has_alpha_val{ false }, mesh_mat{ nullptr } {}

    void setup();
    void draw(Shader& shader);
    void draw_shadows(Shader& shadow_shader);

    mesh(mesh&& m) noexcept : mesh_vertices(std::move(m.mesh_vertices)),
        mesh_centroid(m.mesh_centroid),
        mesh_area(m.mesh_area),
        meshAC(m.meshAC),
        mesh_mat(m.mesh_mat),
        vao(m.vao),
        vbo(m.vbo),
        diffuse_map(m.diffuse_map),
        spec_map(m.spec_map),
        has_alpha_val(m.has_alpha_val)
    {
        m.vao = 0;
        m.vbo = 0;
        m.diffuse_map = 0;
        m.spec_map = 0;
        m.mesh_mat = nullptr;
        m.mesh_area = 0.0f;
        m.meshAC = glm::vec3(0.0f);
        m.has_alpha_val = false;
    }

    ~mesh() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteTextures(1, &diffuse_map);
        glDeleteTextures(1, &spec_map);
    }
};

void ear_clipping(std::vector<vertex>& temp_vertices, mesh& cur_mesh);

class model {
public:
    std::vector<glm::vec3> model_vertices;
    std::vector<glm::vec2> model_texture_vertices;
    std::vector<glm::vec3> model_normals;
    std::vector<mesh> meshes;
    float radius;

    model(const char* model_file_path);
    void unroll_face(std::string& line, int index, mesh& cur_mesh);
    void unroll_v_vt_vn(std::vector<vertex>& temp_vertices, std::string& line, int index);
    void unroll_v_vn(std::vector<vertex>& temp_vertices, std::string& line, int index);
    void parse_mat_file(std::string& mat_file_path, std::string& parent_directory);
    void setup();
    void draw(Shader& shader);
    void draw_shadows(Shader& shadow_shader);
};

model::model(const char* model_file_path) {
    std::ifstream model_file(model_file_path);
    std::string model_file_line;
    mesh cur_mesh = mesh();
    bool first_mesh{ true };
    radius = 0.0f;
    sumAC = glm::vec3(0); true_centroid = glm::vec3(0);
    total_area = 0;
    meshes.clear();
    materials.clear();

    while (getline(model_file, model_file_line)) {
        int line_index = 0;
        std::string line_type(get_line_type(model_file_line, line_index));

        if (line_type == "v") {
            model_vertices.push_back(get_xyz(model_file_line, line_index));
        }
        else if (line_type == "vt") {
            model_texture_vertices.push_back(get_xy(model_file_line, line_index));
        }
        else if (line_type == "vn") {
            model_normals.push_back(get_xyz(model_file_line, line_index));
        }
        else if (line_type == "f") {
            unroll_face(model_file_line, line_index, cur_mesh);
        }
        else if (line_type == "mtllib") {
            std::string parent_path(get_current_directory(model_file_path));
            std::string parent_dir = parent_path;
            std::string mtl_path;

            int i = model_file_line.size() - 1;

            while (i > line_index && is_white_space(model_file_line.at(i))) {
                --i;
            }

            for (line_index; line_index <= i; ++line_index) {
                mtl_path += model_file_line.at(line_index);
            }

            std::filesystem::path mtlPath(mtl_path);

            if (!mtlPath.is_absolute()) {
                parent_path += mtlPath.string();
            }
            else {
                parent_path = mtlPath.string();
            }

            parse_mat_file(parent_path, parent_dir);

        }
        else if (line_type == "usemtl") {
            std::string mat_name;
            int back = model_file_line.size() - 1;

            while (back >= 0 && is_white_space(model_file_line.at(back))) {
                --back;
            }

            for (line_index;line_index <= back;++line_index) {
                mat_name += model_file_line.at(line_index);
            }

            if (first_mesh) {
                first_mesh = false;
                cur_mesh.mesh_mat = &materials[mat_name];
                continue;
            }
            meshes.push_back(std::move(cur_mesh));
            //cur_mesh = mesh();
            cur_mesh.mesh_mat = &materials[mat_name];
        }
        else {

        }
    }

    meshes.push_back(std::move(cur_mesh));
    //cur_mesh = mesh();
    model_texture_vertices.clear();
    model_normals.clear();

    setup();

}

void model::parse_mat_file(std::string& mat_file_path, std::string& parent_directory) {
    std::ifstream mat_file(mat_file_path);
    std::string mat_file_line;
    mat cur_mat = mat();
    bool first_mat{ true };
    std::string cur_mat_name;
    materials["default_mat"] = cur_mat;

    while (getline(mat_file, mat_file_line)) {
        int line_index{ 0 };
        std::string line_type(get_line_type(mat_file_line, line_index));

        if (line_type == "newmtl") {
            int back = mat_file_line.size() - 1;

            if (!first_mat) {
                materials[cur_mat_name] = cur_mat;
                cur_mat_name.clear();
                cur_mat = mat();
            }

            first_mat = false;

            while (back >= 0 && is_white_space(mat_file_line.at(back))) {
                --back;
            }

            for (line_index;line_index <= back;++line_index) {
                cur_mat_name += mat_file_line.at(line_index);
            }

        }
        else if (line_type == "Kd") {
            cur_mat.kd = get_xyz(mat_file_line, line_index);
        }
        else if (line_type == "Ks") {
            cur_mat.ks = get_xyz(mat_file_line, line_index);
        }
        else if (line_type == "Ns") {
            cur_mat.ns = get_x(mat_file_line, line_index);
        }
        else if (line_type == "d") {
            cur_mat.d = get_x(mat_file_line, line_index);
        }
        else if (line_type == "map_Kd" || line_type == "map_Ka") {
            std::string map_path;
            std::string full_map_path = parent_directory;
            int back = mat_file_line.size() - 1;

            while (back >= 0 && is_white_space(mat_file_line.at(back))) {
                --back;
            }

            for (line_index;line_index <= back;++line_index) {
                map_path += mat_file_line.at(line_index);
            }

            std::filesystem::path map_p(map_path);

            if (!map_p.is_absolute()) {
                full_map_path += map_path;
            }
            else {
                full_map_path = map_path;
            }

            stbi_set_flip_vertically_on_load(true);

            int width, height, nr_channels;
            unsigned char* data = stbi_load(full_map_path.data(), &width, &height, &nr_channels, 0);

            if (data) {
                cur_mat.kd_data = data;
                cur_mat.has_kd_map = true;
                cur_mat.kd_nr_channels = nr_channels;
                cur_mat.kd_width = width;
                cur_mat.kd_height = height;
            }
            else {
                std::cout << "failed to load kd_map\n";
            }
        }
        else if (line_type == "map_Ks") {
            std::string map_path;
            std::string full_map_path = parent_directory;
            int back = mat_file_line.size() - 1;

            while (back >= 0 && is_white_space(mat_file_line.at(back))) {
                --back;
            }

            for (line_index;line_index <= back;++line_index) {
                map_path += mat_file_line.at(line_index);
            }

            std::filesystem::path map_p(map_path);

            if (!map_p.is_absolute()) {
                full_map_path += map_path;
            }
            else {
                full_map_path = map_path;
            }

            stbi_set_flip_vertically_on_load(true);

            int width, height, nr_channels;
            unsigned char* data = stbi_load(full_map_path.data(), &width, &height, &nr_channels, 0);

            if (data) {
                cur_mat.ks_data = data;
                cur_mat.has_ks_map = true;
                cur_mat.ks_nr_channels = nr_channels;
                cur_mat.ks_width = width;
                cur_mat.ks_height = height;
            }
            else {
                std::cout << "failed to load ks_map\n";
            }
        }
    }

    materials[cur_mat_name] = cur_mat;
}

void mesh::setup() {
    if (mesh_vertices.size() == 0) {
        return;
    }

    glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * mesh_vertices.size(), &mesh_vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, textureCord));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, vertexNormal));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    if (mesh_mat == nullptr) {
        mesh_mat = &materials["default_mat"];
    }

    if (mesh_mat->has_kd_map) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glGenTextures(1, &diffuse_map);
        glBindTexture(GL_TEXTURE_2D, diffuse_map);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        if (mesh_mat->kd_nr_channels == 3) {
            has_alpha_val = false;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, mesh_mat->kd_width, mesh_mat->kd_height, 0, GL_RGB, GL_UNSIGNED_BYTE, mesh_mat->kd_data);
        }
        else if (mesh_mat->kd_nr_channels == 4) {
            has_alpha_val = true;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, mesh_mat->kd_width, mesh_mat->kd_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mesh_mat->kd_data);
        }

        //glGenerateMipmap(GL_TEXTURE_2D);
    }

    if (mesh_mat->has_ks_map) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glGenTextures(1, &spec_map);
        glBindTexture(GL_TEXTURE_2D, spec_map);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (mesh_mat->ks_nr_channels == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mesh_mat->ks_width, mesh_mat->ks_height, 0, GL_RGB, GL_UNSIGNED_BYTE, mesh_mat->ks_data);
        }
        else if (mesh_mat->ks_nr_channels == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mesh_mat->ks_width, mesh_mat->ks_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mesh_mat->ks_data);
        }

        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

void mesh::draw(Shader& shader) {
    shader.use();
    shader.setVec3("mat.kd", mesh_mat->kd);
    shader.setBool("mat.has_kd_map", mesh_mat->has_kd_map);
    shader.setVec3("mat.ks", mesh_mat->ks);
    shader.setBool("mat.has_ks_map", mesh_mat->has_ks_map);
    shader.setFloat("mat.ns", mesh_mat->ns);
    shader.setBool("mat.has_alpha_value", has_alpha_val);
    shader.setFloat("mat.d", mesh_mat->d);

    if (mesh_mat->has_kd_map) {

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuse_map);
        shader.setInt("mat.diffuse_map", 0);

    }

    if (mesh_mat->has_ks_map) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, spec_map);
        shader.setInt("mat.spec_map", 1);
    }

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh_vertices.size());
    glBindVertexArray(0);
}

void model::setup() {
    for (int i{ 0 }; i < meshes.size();++i) {
        sumAC += meshes.at(i).meshAC;
        total_area += meshes.at(i).mesh_area;
        meshes.at(i).setup();
    }

    true_centroid = sumAC / total_area;

    for (auto& v : model_vertices) {
        radius = std::max(radius, glm::length(v - true_centroid));
    }

    model_vertices.clear();
}

void model::draw(Shader& shader) {
    for (int i{ 0 }; i < meshes.size();++i) {
        meshes.at(i).draw(shader);
    }
}

void model::unroll_face(std::string& line, int index, mesh& cur_mesh) {
    face_type f_type = supported_face_type(line, index);

    if (f_type == unsupported) {
        std::cerr << "unsupported face type: " << line << '\n';
        return;
    }


    std::vector<vertex> temp_vertices;
    line += ' ';
    if (f_type == v_vt_vn) {
        unroll_v_vt_vn(temp_vertices, line, index);
    }
    else if (f_type == v_vn) {
        unroll_v_vn(temp_vertices, line, index);
    }

    if (temp_vertices.size() == 3) {

        for (int i = 0;i < temp_vertices.size();++i) {
            cur_mesh.mesh_vertices.push_back(temp_vertices.at(i));
        }

        float temp_area = 0.5f * glm::length(glm::cross((temp_vertices.at(2).vertexCord - temp_vertices.at(0).vertexCord), (temp_vertices.at(1).vertexCord - temp_vertices.at(0).vertexCord)));
        glm::vec3 temp_center = (temp_vertices.at(0).vertexCord + temp_vertices.at(1).vertexCord + temp_vertices.at(2).vertexCord) / 3.0f;

        cur_mesh.meshAC += temp_area * temp_center;
        cur_mesh.mesh_area += temp_area;
    }
    else {
        ear_clipping(temp_vertices, cur_mesh);
    }
}

void model::unroll_v_vn(std::vector<vertex>& temp_vertices, std::string& line, int index) {
    std::string indicies[2];
    int count = 0;
    while (index < line.size()) {
        if (is_white_space(line.at(index))) {

            int x = std::stoi(indicies[0]); int y = std::stoi(indicies[1]);

            if (x > 0)
                x -= 1;
            else
                x = model_vertices.size() + x;

            if (y > 0)
                y -= 1;
            else
                y = model_normals.size() + y;

            temp_vertices.emplace_back(vertex(
                model_vertices[x], glm::vec2(0), model_normals[y]
            ));

            skip_white_space_forward(line, index);

            count = 0;

            indicies[0].clear(); indicies[1].clear();
        }
        else if (line.at(index) == '/') {
            index += 2;
            ++count;
        }
        else {
            indicies[count] += line.at(index);
            ++index;
        }
    }
}

void model::unroll_v_vt_vn(std::vector<vertex>& temp_vertices, std::string& line, int index) {
    std::string indicies[3];
    int count = 0;
    while (index < line.size()) {
        if (is_white_space(line.at(index))) {

            int x = std::stoi(indicies[0]); int y = std::stoi(indicies[1]); int z = std::stoi(indicies[2]);

            if (x > 0)
                x -= 1;
            else
                x = model_vertices.size() + x;

            if (y > 0)
                y -= 1;
            else
                y = model_texture_vertices.size() + y;

            if (z > 0)
                z -= 1;
            else
                z = model_normals.size() + z;

            temp_vertices.emplace_back(vertex(
                model_vertices[x], model_texture_vertices[y], model_normals[z]
            ));

            skip_white_space_forward(line, index);

            count = 0;

            indicies[0].clear(); indicies[1].clear(); indicies[2].clear();
        }
        else if (line.at(index) == '/') {
            ++index;
            ++count;
        }
        else {
            indicies[count] += line.at(index);
            ++index;
        }

    }
}

void ear_clipping(std::vector<vertex>& temp_vertices, mesh& cur_mesh) {
    //calc surface normal to determine dominate plane
    glm::vec3 surface_normal(0);

    for (int i = 0; i < temp_vertices.size();++i) {
        glm::vec3 current = temp_vertices.at(i).vertexCord;
        glm::vec3 next = temp_vertices.at((i + 1) % temp_vertices.size()).vertexCord;

        surface_normal.x += (current.y - next.y) * (current.z + next.z);
        surface_normal.y += (current.z - next.z) * (current.x + next.x);
        surface_normal.z += (current.x - next.x) * (current.y + next.y);
    }

    surface_normal = glm::normalize(surface_normal);

    float x = abs(surface_normal.x); float y = abs(surface_normal.y); float z = abs(surface_normal.z);

    int index_one, index_two;

    if (x >= y && x >= z) {
        index_one = 1; index_two = 2;
    }
    else if (y >= z && y >= x) {
        index_one = 0; index_two = 2;
    }
    else {
        index_one = 0; index_two = 1;
    }

    float signed_area = 0;

    for (int i = 0; i < temp_vertices.size();++i) {
        glm::vec3 current = temp_vertices.at(i).vertexCord;
        glm::vec3 next = temp_vertices.at((i + 1) % temp_vertices.size()).vertexCord;

        signed_area += (next[index_one] - current[index_one]) * (next[index_two] + current[index_two]);
    }

    if (signed_area > 0) {
        reverse(temp_vertices.begin(), temp_vertices.end());
    }

    while (temp_vertices.size() > 3) {
        bool infinite_loop = true;

        for (int i = 0; i < temp_vertices.size();++i) {
            glm::vec2 prev(temp_vertices.at(i).vertexCord[index_one], temp_vertices.at(i).vertexCord[index_two]);
            glm::vec2 current(temp_vertices.at((i + 1) % temp_vertices.size()).vertexCord[index_one], temp_vertices.at((i + 1) % temp_vertices.size()).vertexCord[index_two]);
            glm::vec2 next(temp_vertices.at((i + 2) % temp_vertices.size()).vertexCord[index_one], temp_vertices.at((i + 2) % temp_vertices.size()).vertexCord[index_two]);

            glm::vec2 current_prev(prev - current);
            glm::vec2 current_next(next - current);

            float cross = (current_prev.x * current_next.y) - (current_prev.y * current_next.x);
            bool is_ear = true;
            if (cross > 0) {
                for (int j = 0;j < temp_vertices.size();++j) {
                    if (j == i || j == i + 1 || j == i + 2) {
                        continue;
                    }
                    glm::vec2 point_to_test(temp_vertices.at(j).vertexCord[index_one], temp_vertices.at(j).vertexCord[index_two]);

                    glm::vec3 point_to_test2(temp_vertices.at(j).vertexCord);

                    float d1 = (point_to_test.x - current.x) * (prev.y - current.y) - (prev.x - current.x) * (point_to_test.y - current.y);
                    float d2 = (point_to_test.x - next.x) * (current.y - next.y) - (current.x - next.x) * (point_to_test.y - next.y);
                    float d3 = (point_to_test.x - prev.x) * (next.y - prev.y) - (next.x - prev.x) * (point_to_test.y - prev.y);

                    if (d1 >= 0 && d2 >= 0 && d3 >= 0) {
                        std::cout << "not an ear\n";
                        is_ear = false;
                        break;
                    }
                }
            }

            if (is_ear) {
                cur_mesh.mesh_vertices.push_back(temp_vertices.at(i));
                cur_mesh.mesh_vertices.push_back(temp_vertices.at((i + 1) % temp_vertices.size()));
                cur_mesh.mesh_vertices.push_back(temp_vertices.at((i + 2) % temp_vertices.size()));

                float temp_area = 0.5f * glm::length(glm::cross((temp_vertices.at((i + 1) % temp_vertices.size()).vertexCord - temp_vertices.at(i).vertexCord), (temp_vertices.at((i + 2) % temp_vertices.size()).vertexCord - temp_vertices.at(i).vertexCord)));
                glm::vec3 temp_center = (temp_vertices.at(i).vertexCord + temp_vertices.at((i + 1) % temp_vertices.size()).vertexCord + temp_vertices.at((i + 2) % temp_vertices.size()).vertexCord) / 3.0f;

                cur_mesh.meshAC += temp_area * temp_center;
                cur_mesh.mesh_area += temp_area;

                temp_vertices.erase(temp_vertices.begin() + ((i + 1) % temp_vertices.size()));
                infinite_loop = false;
                break;
            }
            else {
                std::cout << "ear not found\n";
            }

        }

        if (infinite_loop) {
            std::cout << "infinite loop detected\n";
            return;
        }
    }

    for (int i = 0;i < temp_vertices.size();++i) {
        cur_mesh.mesh_vertices.push_back(temp_vertices.at(i));
    }

    float temp_area = 0.5f * glm::length(glm::cross((temp_vertices.at(2).vertexCord - temp_vertices.at(0).vertexCord), (temp_vertices.at(1).vertexCord - temp_vertices.at(0).vertexCord)));
    glm::vec3 temp_center = (temp_vertices.at(0).vertexCord + temp_vertices.at(1).vertexCord + temp_vertices.at(2).vertexCord) / 3.0f;

    cur_mesh.meshAC += temp_area * temp_center;
    cur_mesh.mesh_area += temp_area;
}

std::string get_line_type(std::string& line, int& index) {
    std::string line_type;

    skip_white_space_forward(line, index);

    while (index < line.size() && !is_white_space(line.at(index))) {
        line_type.push_back(line.at(index));
        ++index;
    }

    skip_white_space_forward(line, index);

    return line_type;
}

std::string get_current_directory(const char* model_file_path) {
    return std::filesystem::path(model_file_path).parent_path().string() + '/';
}

float get_x(std::string& line, int index) {
    float temp = 1.0;
    std::string num;

    while (index < line.size()) {
        if (line.at(index) == ' ') {
            temp = std::stof(num);
            num.clear();
            break;
        }
        else {
            num += line.at(index);
            ++index;
        }
    }

    if (!num.empty()) {
        temp = std::stof(num);
    }

    return temp;
}

glm::vec2 get_xy(std::string& line, int index) {
    glm::vec2 temp_vert(0);
    int count = 0;
    std::string num;

    while (index < line.size() && count < 2) {
        if (line.at(index) == ' ') {
            temp_vert[count] = std::stod(num);
            ++count;
            num.clear();

            skip_white_space_forward(line, index);
        }
        else {
            num += line.at(index);
            ++index;
        }
    }

    if (!num.empty() && count < 2)
        temp_vert[count] = std::stof(num);

    return temp_vert;
}

glm::vec3 get_xyz(std::string& line, int index) {
    glm::vec3 temp_vert(0);
    int count = 0;// only want x, y , z
    std::string num;

    while (index < line.size() && count < 3) {
        if (is_white_space(line.at(index))) {
            if (!num.empty()) {
                temp_vert[count] = std::stod(num);
                ++count;
                num.clear();
            }

            skip_white_space_forward(line, index);
        }
        else {
            num += line.at(index);
            ++index;
        }
    }

    if (!num.empty() && count < 3)
        temp_vert[count] = std::stod(num);

    return temp_vert;
}

face_type supported_face_type(std::string& line, int temp_index) {
    int slash_count = 0;
    int first_slash_index = 0;
    face_type f_type = unsupported;

    while (temp_index < line.size() && !is_white_space(line.at(temp_index)) && slash_count < 2) {
        if (line.at(temp_index) == '/' && slash_count == 0) {
            first_slash_index = temp_index;
            ++slash_count;
        }
        else if (line.at(temp_index) == '/') {
            if (temp_index - first_slash_index != 1) {
                f_type = v_vt_vn;
                break;
            }
            else if (temp_index - first_slash_index == 1) {
                f_type = v_vn;
                break;
            }
            ++slash_count;
        }
        ++temp_index;
    }

    return f_type;

}

void skip_white_space_forward(std::string& line, int& index) {
    while (index < line.size() && is_white_space(line.at(index)))
        ++index;
}

bool is_white_space(char c) {
    return std::isspace(static_cast<unsigned int>(c));
}