#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <glm/glm.hpp>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <memory>

bool is_white_space(char c) {
    return std::isspace(static_cast<unsigned int>(c));
}

void skip_white_space(const std::string& line, size_t& index, bool backwards = false) {
    if (!backwards) {
        while (index < line.size() && is_white_space(line.at(index)))
            ++index;
    }
    else {
        while (index > 0 && is_white_space(line.at(index)))
            --index;
    }
}

std::string get_line_type(const std::string& line, size_t& index) {
    std::string line_type;

    skip_white_space(line, index);

    while (index < line.size() && !is_white_space(line.at(index))) {
        line_type.push_back(line.at(index));
        ++index;
    }

    skip_white_space(line, index);

    return line_type;
}

float get_value(const std::string& line, size_t index) {
    float temp = 1.0;
    std::string num;

    while (index < line.size()) {
        if (is_white_space(line.at(index))) {
            temp = static_cast<float>(std::stod(num));
            num.clear();
            break;
        }
        else {
            num += line.at(index);
            ++index;
        }
    }

    if (!num.empty()) {
        temp = static_cast<float>(std::stod(num));
    }

    return temp;
}

template <typename glm_vec, size_t N>
glm_vec get_values(const std::string& line, size_t index) {
    glm_vec temp{};
    std::string num;
    size_t count = 0;

    while (index < line.size() && count < N) {
        if (is_white_space(line.at(index))) {
            temp[count] = static_cast<float>(std::stod(num));
            ++count;
            num.clear();

            skip_white_space(line, index);
        }
        else {
            num += line.at(index);
            ++index;
        }
    }

    if (!num.empty() && count < N)
        temp[count] = static_cast<float>(std::stod(num));

    return temp;
}

enum class face_type {
    v_vt_vn,
    v_vn,
    unsupported
};

face_type supported_face_type(const std::string& line, size_t index) {
    int slash_count = 0;
    size_t first_slash_index = 0;
    face_type f_type = face_type::unsupported;

    while (index < line.size() && !is_white_space(line.at(index)) && slash_count < 2) {
        if (line.at(index) == '/' && slash_count == 0) {
            first_slash_index = index;
            ++slash_count;
        }
        else if (line.at(index) == '/') {
            if (index - first_slash_index != 1) {
                f_type = face_type::v_vt_vn;
                break;
            }
            else if (index - first_slash_index == 1) {
                f_type = face_type::v_vn;
                break;
            }
            ++slash_count;
        }
        ++index;
    }

    return f_type;
}

class vertex {
public:
    glm::vec3 vertex_coord;
    glm::vec2 texture_coord;
    glm::vec3 vertex_normal;

    vertex(const glm::vec3& vertex_coord, const glm::vec2& texture_coord, const glm::vec3& vertex_normal) :
        vertex_coord(vertex_coord), texture_coord(texture_coord), vertex_normal(vertex_normal) {
    }
};

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

    void reset() {
        kd = glm::vec3(0.2); ks = glm::vec3(1.0);
        ns = 32.0; d = 1.0;
        has_kd_map = false; has_ks_map = false;
        kd_data = nullptr; ks_data = nullptr;
        kd_width = 0; kd_height = 0;
        ks_width = 0; ks_height = 0;
        kd_nr_channels = 0; ks_nr_channels = 0;
    }
};

std::unordered_map<std::string, mat> materials;

class mesh {
public:
    std::vector<vertex> mesh_vertices;
    mat* mesh_mat;
    bool has_alpha_val;
    unsigned int vao, vbo, diffuse_map, spec_map;
    mesh() : vao(0), vbo(0), diffuse_map(0), spec_map(0), mesh_mat{ nullptr }, has_alpha_val{ false } {}

    void setup();
    void draw(Shader& shader);

    mesh(const mesh&) = delete;
    mesh& operator=(const mesh&) = delete;

    mesh(mesh&& rhs) {
        mesh_vertices = std::move(rhs.mesh_vertices);
        mesh_mat = rhs.mesh_mat;
        has_alpha_val = rhs.has_alpha_val;
        vao = rhs.vao; vbo = rhs.vbo;
        diffuse_map = rhs.diffuse_map; spec_map = rhs.spec_map;

        rhs.vao = 0; rhs.vbo = 0;
        rhs.diffuse_map = 0; rhs.spec_map = 0;
        rhs.mesh_mat = nullptr;
    }
    mesh& operator=(mesh&& rhs) {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteTextures(1, &diffuse_map);
        glDeleteTextures(1, &spec_map);
        mesh_vertices = std::move(rhs.mesh_vertices);
        mesh_mat = rhs.mesh_mat;
        has_alpha_val = rhs.has_alpha_val;
        vao = rhs.vao; vbo = rhs.vbo;
        diffuse_map = rhs.diffuse_map; spec_map = rhs.spec_map;

        rhs.vao = 0; rhs.vbo = 0;
        rhs.diffuse_map = 0; rhs.spec_map = 0;
        rhs.mesh_mat = nullptr;

        return *this;
    }

    ~mesh() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteTextures(1, &diffuse_map);
        glDeleteTextures(1, &spec_map);
    }
};

void mesh::setup() {
    if (mesh_vertices.size() == 0) {
        return;
    }

    glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * mesh_vertices.size(), &mesh_vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, texture_coord));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, vertex_normal));
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

class model {
public:
    std::vector<glm::vec3> model_vertices;
    std::vector<glm::vec2> model_texture_vertices;
    std::vector<glm::vec3> model_normals;
    std::vector<mesh> meshes;
    float model_area;
    glm::vec3 model_ac, centroid;
    std::string parent_dir;
    float radius;

    model(const std::string& model_file_path);
    void unroll_face(std::string& line, size_t index);
    void unroll_v_vn(std::vector<vertex>& temp_vertices, std::string& line, size_t index);
    void unroll_v_vt_vn(std::vector<vertex>& temp_vertices, std::string& line, size_t index);
    void ear_clipping(std::vector<vertex>& temp_vertices, mesh& cur_mesh);
    std::string get_file_path(const std::string& line, size_t index);
    void parse_mat_file(const std::string& mat_file_path);
    void setup();
    void draw(Shader& shader);

    model(const model&) = delete;
    model& operator=(const model&) = delete;

    model(model&& rhs) = default;
    model& operator=(model&& rhs) = default;
};

void model::unroll_v_vn(std::vector<vertex>& temp_vertices, std::string& line, size_t index) {
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

            skip_white_space(line, index);

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

void model::unroll_v_vt_vn(std::vector<vertex>& temp_vertices, std::string& line, size_t index) {
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

            skip_white_space(line, index);

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

void model::unroll_face(std::string& line, size_t index) {
    face_type f_type{ supported_face_type(line, index) };

    if (f_type == face_type::unsupported) {
        std::cerr << "unsupported face type: " << line << '\n';
        return;
    }

    std::vector<vertex> temp_vertices;
    line += ' ';
    if (f_type == face_type::v_vt_vn)
        unroll_v_vt_vn(temp_vertices, line, index);
    else if (f_type == face_type::v_vn)
        unroll_v_vn(temp_vertices, line, index);

    if (temp_vertices.size() == 3) {

        for (auto i = 0; i < 3; ++i) {
            meshes.back().mesh_vertices.emplace_back(temp_vertices.at(i));
        }

        float temp_area = 0.5f * glm::length(glm::cross((temp_vertices.at(2).vertex_coord - temp_vertices.at(0).vertex_coord), (temp_vertices.at(1).vertex_coord - temp_vertices.at(0).vertex_coord)));
        glm::vec3 temp_center = (temp_vertices.at(0).vertex_coord + temp_vertices.at(1).vertex_coord + temp_vertices.at(2).vertex_coord) / 3.0f;

        model_ac += temp_area * temp_center;
        model_area += temp_area;
    }
    else if (temp_vertices.size() > 3)
        ear_clipping(temp_vertices, meshes.back());
}

std::string model::get_file_path(const std::string& line, size_t index) {
    std::string mtl_path;

    size_t index_back = line.size() - 1;

    skip_white_space(line, index_back, true);

    for (index; index <= index_back; ++index) {
        mtl_path += line.at(index);
    }

    std::filesystem::path temp(mtl_path);

    if (!temp.is_absolute())
        return parent_dir + temp.string();
    else
        return temp.string();
}

void model::parse_mat_file(const std::string& mat_file_path) {
    std::ifstream mat_file(mat_file_path);
    std::string mat_file_line;

    std::string cur_mat_name;
    bool first_mat = true;
    mat temp_mat{};

    materials.insert({ "default_mat", temp_mat });

    while (getline(mat_file, mat_file_line)) {
        if (mat_file_line.empty())
            continue;
        size_t line_index = 0;
        std::string line_type(get_line_type(mat_file_line, line_index));

        if (line_type == "newmtl") {
            size_t index_back = mat_file_line.size() - 1;

            if (!first_mat) {
                materials[cur_mat_name] = temp_mat;
                cur_mat_name.clear();
                temp_mat.reset();
            }

            first_mat = false;

            skip_white_space(mat_file_line, index_back, true);

            for (line_index; line_index <= index_back; ++line_index) {
                cur_mat_name += mat_file_line.at(line_index);
            }
        }
        else if (line_type == "Kd") {
            temp_mat.kd = get_values<glm::vec3, 3>(mat_file_line, line_index);
        }
        else if (line_type == "Ks") {
            temp_mat.ks = get_values<glm::vec3, 3>(mat_file_line, line_index);
        }
        else if (line_type == "Ns") {
            temp_mat.ns = get_value(mat_file_line, line_index);
        }
        else if (line_type == "d") {
            temp_mat.d = get_value(mat_file_line, line_index);
        }
        else if (line_type == "map_Ka" || line_type == "map_Kd") {
            std::string map_path = get_file_path(mat_file_line, line_index);

            stbi_set_flip_vertically_on_load(true);

            int width, height, nr_channels;
            unsigned char* data = stbi_load(map_path.data(), &width, &height, &nr_channels, 0);

            if (data) {
                temp_mat.kd_data = data;
                temp_mat.has_kd_map = true;
                temp_mat.kd_nr_channels = nr_channels;
                temp_mat.kd_width = width;
                temp_mat.kd_height = height;
            }
            else {
                std::cout << "failed to load kd_map on line: " << line_type << std::endl;
            }
        }
        else if (line_type == "map_Ks") {
            std::string map_path = get_file_path(mat_file_line, line_index);

            stbi_set_flip_vertically_on_load(true);

            int width, height, nr_channels;
            unsigned char* data = stbi_load(map_path.data(), &width, &height, &nr_channels, 0);

            if (data) {
                temp_mat.ks_data = data;
                temp_mat.has_ks_map = true;
                temp_mat.ks_nr_channels = nr_channels;
                temp_mat.ks_width = width;
                temp_mat.ks_height = height;
            }
            else {
                std::cout << "failed to load ks_map on line: " << mat_file_line << std::endl;
            }
        }
    }

    materials[cur_mat_name] = temp_mat;
}

void model::ear_clipping(std::vector<vertex>& temp_vertices, mesh& cur_mesh) {
    //calc surface normal to determine dominate plane
    glm::vec3 surface_normal(0);

    for (int i = 0; i < temp_vertices.size();++i) {
        glm::vec3 current = temp_vertices.at(i).vertex_coord;
        glm::vec3 next = temp_vertices.at((i + 1) % temp_vertices.size()).vertex_coord;

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
        glm::vec3 current = temp_vertices.at(i).vertex_coord;
        glm::vec3 next = temp_vertices.at((i + 1) % temp_vertices.size()).vertex_coord;

        signed_area += (next[index_one] - current[index_one]) * (next[index_two] + current[index_two]);
    }

    if (signed_area > 0) {
        reverse(temp_vertices.begin(), temp_vertices.end());
    }

    while (temp_vertices.size() > 3) {
        bool infinite_loop = true;

        for (int i = 0; i < temp_vertices.size();++i) {
            glm::vec2 prev(temp_vertices.at(i).vertex_coord[index_one], temp_vertices.at(i).vertex_coord[index_two]);
            glm::vec2 current(temp_vertices.at((i + 1) % temp_vertices.size()).vertex_coord[index_one], temp_vertices.at((i + 1) % temp_vertices.size()).vertex_coord[index_two]);
            glm::vec2 next(temp_vertices.at((i + 2) % temp_vertices.size()).vertex_coord[index_one], temp_vertices.at((i + 2) % temp_vertices.size()).vertex_coord[index_two]);

            glm::vec2 current_prev(prev - current);
            glm::vec2 current_next(next - current);

            float cross = (current_prev.x * current_next.y) - (current_prev.y * current_next.x);
            bool is_ear = true;
            if (cross > 0) {
                for (int j = 0;j < temp_vertices.size();++j) {
                    if (j == i || j == i + 1 || j == i + 2) {
                        continue;
                    }
                    glm::vec2 point_to_test(temp_vertices.at(j).vertex_coord[index_one], temp_vertices.at(j).vertex_coord[index_two]);

                    glm::vec3 point_to_test2(temp_vertices.at(j).vertex_coord);

                    float d1 = (point_to_test.x - current.x) * (prev.y - current.y) - (prev.x - current.x) * (point_to_test.y - current.y);
                    float d2 = (point_to_test.x - next.x) * (current.y - next.y) - (current.x - next.x) * (point_to_test.y - next.y);
                    float d3 = (point_to_test.x - prev.x) * (next.y - prev.y) - (next.x - prev.x) * (point_to_test.y - prev.y);

                    if (d1 >= 0 && d2 >= 0 && d3 >= 0) {
                        is_ear = false;
                        break;
                    }
                }
            }

            if (is_ear) {
                cur_mesh.mesh_vertices.push_back(temp_vertices.at(i));
                cur_mesh.mesh_vertices.push_back(temp_vertices.at((i + 1) % temp_vertices.size()));
                cur_mesh.mesh_vertices.push_back(temp_vertices.at((i + 2) % temp_vertices.size()));

                float temp_area = 0.5f * glm::length(glm::cross((temp_vertices.at((i + 1) % temp_vertices.size()).vertex_coord - temp_vertices.at(i).vertex_coord), (temp_vertices.at((i + 2) % temp_vertices.size()).vertex_coord - temp_vertices.at(i).vertex_coord)));
                glm::vec3 temp_center = (temp_vertices.at(i).vertex_coord + temp_vertices.at((i + 1) % temp_vertices.size()).vertex_coord + temp_vertices.at((i + 2) % temp_vertices.size()).vertex_coord) / 3.0f;

                model_ac += temp_area * temp_center;
                model_area += temp_area;

                temp_vertices.erase(temp_vertices.begin() + ((i + 1) % temp_vertices.size()));
                infinite_loop = false;
                break;
            }

        }

        if (infinite_loop) {
            return;
        }
    }

    for (int i = 0;i < temp_vertices.size();++i) {
        cur_mesh.mesh_vertices.push_back(temp_vertices.at(i));
    }

    float temp_area = 0.5f * glm::length(glm::cross((temp_vertices.at(2).vertex_coord - temp_vertices.at(0).vertex_coord), (temp_vertices.at(1).vertex_coord - temp_vertices.at(0).vertex_coord)));
    glm::vec3 temp_center = (temp_vertices.at(0).vertex_coord + temp_vertices.at(1).vertex_coord + temp_vertices.at(2).vertex_coord) / 3.0f;

    model_ac += temp_area * temp_center;
    model_area += temp_area;
}

void model::setup() {
    for (auto i = 0; i < meshes.size(); ++i) {
        meshes.at(i).setup();
    }

    centroid = model_ac / model_area;

    for (auto& v : model_vertices) {
        radius = std::max(radius, glm::length(v - centroid));
    }

    model_vertices.clear();
    model_texture_vertices.clear();
    model_normals.clear();
}

void model::draw(Shader& shader) {
    for (auto i = 0; i < meshes.size(); ++i) {
        meshes.at(i).draw(shader);
    }
}

model::model(const std::string& model_file_path) {
    materials.clear();
    std::ifstream model_file(model_file_path);
    std::string model_file_line;
    parent_dir = std::filesystem::path(model_file_path).parent_path().string() + '/';
    bool first_mesh = true;
    model_ac = glm::vec3(0.0f); model_area = 0.0f; centroid = glm::vec3(0.0f);
    radius = 0.0f;

    meshes.emplace_back(mesh());

    while (getline(model_file, model_file_line)) {
        if (model_file_line.empty())
            continue;
        size_t line_index = 0;

        std::string line_type = get_line_type(model_file_line, line_index);

        if (line_type == "v") {
            model_vertices.emplace_back(get_values<glm::vec3, 3>(model_file_line, line_index));
        }
        else if (line_type == "vt") {
            model_texture_vertices.emplace_back(get_values<glm::vec2, 2>(model_file_line, line_index));
        }
        else if (line_type == "vn") {
            model_normals.emplace_back(get_values<glm::vec3, 3>(model_file_line, line_index));
        }
        else if (line_type == "f") {
            unroll_face(model_file_line, line_index);
        }
        else if (line_type == "mtllib") {
            parse_mat_file(get_file_path(model_file_line, line_index));
        }
        else if (line_type == "usemtl") {
            std::string mat_name;
            size_t index_back = model_file_line.size() - 1;

            skip_white_space(model_file_line, index_back, true);

            for (line_index;line_index <= index_back;++line_index) {
                mat_name += model_file_line.at(line_index);
            }

            if (first_mesh) {
                first_mesh = false;
                meshes.back().mesh_mat = &materials[mat_name];
                continue;
            }

            meshes.push_back(mesh());

            meshes.back().mesh_mat = &materials[mat_name];
        }
    }
    setup();
}