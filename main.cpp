#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cmath>
#include <windows.h>
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "shader.hpp"
#include "cam.hpp"
#include "hamood_obj_loader.hpp"


int window_width = 1000, window_height = 1000;
float yaw = 0.0f, pitch = 0.0f;
double last_x, last_y;
bool left_button_pressed = false;
bool input_button_pressed = false;
bool swap_model = false;
unsigned int prev_depth, color_attachment, cur_depth;
int layers = 10;
OPENFILENAMEA f = { sizeof(OPENFILENAMEA) };
unsigned int peel_depths[2];
std::vector<unsigned int> color_attachments(layers);
std::string model_name = std::filesystem::current_path().parent_path().string() + "/default_model/bunny.obj";
std::string main_shader_vs_path = std::filesystem::current_path().parent_path().string() + "/shaders/shader.vs";
std::string main_shader_fs_path = std::filesystem::current_path().parent_path().string() + "/shaders/shader.fs";
std::string button_shader_vs = std::filesystem::current_path().parent_path().string() + "/shaders/input_button_shader.vs";
std::string button_shader_fs = std::filesystem::current_path().parent_path().string() + "/shaders/input_button_shader.fs";
std::string screen_shader_vs = std::filesystem::current_path().parent_path().string() + "/shaders/screen.vs";
std::string screen_shader_fs = std::filesystem::current_path().parent_path().string() + "/shaders/screen.fs";

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
GLFWwindow* glfwSetup();
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

orbit_camera orbit_cam(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 3.0f, 0.0f, 0.0f);

int main() {
    GLFWwindow* window = glfwSetup();

    Shader main_shader(main_shader_vs_path.c_str(), main_shader_fs_path.c_str());
    Shader input_button_shader(button_shader_vs.c_str(), button_shader_fs.c_str());
    Shader screen_shader(screen_shader_vs.c_str(), screen_shader_fs.c_str());
    model m(model_name.c_str());
    //modeler mer(model_name);
    f.lpstrFilter = "obj files\0*.obj\0";
    f.lpstrTitle = "Select Obj File";
    char buff[MAX_PATH] = {};
    f.nMaxFile = sizeof(buff);
    f.lpstrFile = buff;



    float input_button[] = {
        -0.9, 0.9, 0, 0, 1,
        -0.9, 0.8, 0, 0, 0,
        -0.7, 0.9, 0, 1, 1,
        -0.7, 0.8, 0, 1, 0
    };

    float quad_vertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    unsigned int quad_VAO, quad_VBO;
    glGenVertexArrays(1, &quad_VAO);
    glGenBuffers(1, &quad_VBO);
    glBindVertexArray(quad_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, quad_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    unsigned int input_button_vao, input_button_vbo;
    glGenVertexArrays(1, &input_button_vao); glGenBuffers(1, &input_button_vbo);
    glBindVertexArray(input_button_vao); glBindBuffer(GL_ARRAY_BUFFER, input_button_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(input_button), input_button, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    input_button_shader.use();
    stbi_set_flip_vertically_on_load(true);

    int width, height, nr_channels;
    std::string input_img_path = std::filesystem::current_path().parent_path().string() + "/input_button.png";
    unsigned char* data = stbi_load(input_img_path.c_str(), &width, &height, &nr_channels, 0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    unsigned int input_button_texture;
    glGenTextures(1, &input_button_texture);
    glBindTexture(GL_TEXTURE_2D, input_button_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input_button_texture);
    input_button_shader.setInt("button_tex", 0);

    unsigned int depth_peeling_FBO;
    glGenFramebuffers(1, &depth_peeling_FBO);
    glGenTextures(1, &prev_depth); glGenTextures(1, &cur_depth);
    glBindFramebuffer(GL_FRAMEBUFFER, depth_peeling_FBO);

    glGenTextures(layers, color_attachments.data());
    for (int i = 0; i < layers; ++i) {
        glBindTexture(GL_TEXTURE_2D, color_attachments[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_attachments[0], 0);
    glGenTextures(2, peel_depths);

    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, peel_depths[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, window_width, window_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    while (!glfwWindowShouldClose(window)) {
        bool first_pass = true;

        if (swap_model) {
            m = model(f.lpstrFile);
            swap_model = false;
        }

        if (window_width == 0 || window_height == 0) {
            glfwWaitEvents();
            continue;
        }

        glViewport(0, 0, window_width, window_height);

        float scale = 2.0f / m.radius;
        glm::mat4 Model(1.0f);
        Model = glm::scale(Model, glm::vec3(scale));
        Model = glm::translate(Model, -m.centroid);
        //Model = glm::translate(Model, -true_centroid);

        glEnable(GL_DEPTH_TEST);

        glBindFramebuffer(GL_FRAMEBUFFER, depth_peeling_FBO);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
        glClearDepth(1.0f);
        glDepthMask(GL_TRUE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


        orbit_cam.rotate_x(glm::radians(yaw));
        orbit_cam.rotate_y(glm::radians(pitch));

        glm::mat4 projection = glm::perspective(glm::radians(90.0f), (float)window_width / (float)window_height, 0.1f, 100.0f);
        main_shader.use();

        main_shader.setMat4("model", Model);
        main_shader.setMat4("view", orbit_cam.get_view_matrix());
        main_shader.setMat4("projection", projection);
        main_shader.setVec3("light_location", glm::vec3(0.0, 25, 0.0));
        main_shader.setVec2("screen_size", glm::vec2(window_width, window_height));
        main_shader.setBool("first_pass", true);


        glDisable(GL_BLEND);

        for (int i = 0; i < layers;++i) {
            unsigned int curDepth = peel_depths[i % 2];
            unsigned int prevDepth = peel_depths[(i + 1) % 2];

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_attachments[i], 0);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, curDepth, 0);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (i > 0) {
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, prevDepth);
                main_shader.setInt("prev_depth", 3);
            }

            main_shader.setBool("first_pass", (i == 0));
            m.draw(main_shader);
        }

        glEnable(GL_BLEND);

        yaw = 0;
        pitch = 0;

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        screen_shader.use();
        glBindVertexArray(quad_VAO);
        screen_shader.setVec2("screen_size", glm::vec2(window_width, window_height));

        for (int i = layers - 1; i >= 0; --i) {
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, color_attachments[i]);
            screen_shader.setInt("screenTexture", 5);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        input_button_shader.use();
        glBindVertexArray(input_button_vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, input_button_texture);
        input_button_shader.setInt("button_tex", 0);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    window_width = width;
    window_height = height;
    for (int i = 0; i < layers; ++i) {
        glBindTexture(GL_TEXTURE_2D, color_attachments[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }

    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, peel_depths[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, window_width, window_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS && !swap_model) {
            double click_x, click_y;
            glfwGetCursorPos(window, &click_x, &click_y);
            click_x = (click_x / window_width) * 2.0f - 1.0f;
            click_y = 1.0f - (click_y / window_height) * 2.0f;
            if (click_x > -0.9f && click_x < -0.7f && click_y < 0.9f && click_y > 0.8f) {
                input_button_pressed = true;
                GetOpenFileNameA(&f);
                std::string temp = f.lpstrFile;
                if (temp.size() > 0)
                    swap_model = true;
                action = GLFW_RELEASE;
            }
            else if (!input_button_pressed) {
                left_button_pressed = true;
                glfwGetCursorPos(window, &last_x, &last_y);
            }
        }
        else if (action == GLFW_RELEASE) {
            left_button_pressed = false;
            input_button_pressed = false;
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    orbit_cam.radius -= (float)yoffset;
    if (orbit_cam.radius < 1.0f)
        orbit_cam.radius = 1.0f;
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!left_button_pressed) {
        return;
    }

    yaw += (xpos - last_x) * 0.05;
    pitch += (ypos - last_y) * 0.05;

    yaw = std::fmod(yaw, 360.0f);
    pitch = std::fmod(pitch, 360.0f);

    last_x = xpos;
    last_y = ypos;
}

GLFWwindow* glfwSetup() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Hamood_Viewer", NULL, NULL);
    if (window == NULL) {
        std::cout << "GLFW window creation failed\n";
        return NULL;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "GLAD initialization failed\n";
        return NULL;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glViewport(0, 0, window_width, window_height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    return window;
}