#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class orbit_camera {
public:
    glm::vec3 center;
    glm::vec3 world_up;
    float radius;
    float x_angle;
    float y_angle;
    orbit_camera(const glm::vec3& center, const glm::vec3& up, float radius, float x_angle, float y_angle) :
        center{ center }, world_up{ up }, radius{ radius }, x_angle{ x_angle }, y_angle{ y_angle } {
    }

    void rotate_x(const float radians) {
        x_angle += radians;

        const float full_circle = 2.0f * glm::pi<float>();

        x_angle = std::fmod(x_angle, full_circle);

        if (x_angle < 0.0f)
            x_angle = full_circle + x_angle;
    }

    void rotate_y(const float radians) {
        y_angle += radians;

        const float y_cap = glm::pi<float>() / 2.0f - 0.001f;
        if (y_angle > y_cap)
            y_angle = y_cap;

        if (y_angle < -y_cap)
            y_angle = -y_cap;
    }

    glm::vec3 get_eye() {
        const auto sin_x = sin(x_angle);
        const auto cosin_x = cos(x_angle);
        const auto sin_y = sin(y_angle);
        const auto cosin_y = cos(y_angle);

        // Calculate eye position out of them
        const auto x = center.x + radius * cosin_y * cosin_x;
        const auto y = center.y + radius * sin_y;
        const auto z = center.z + radius * cosin_y * sin_x;

        return glm::vec3(x, y, z);
    }

    void mouse_scroll(float offset) {

    }

    glm::mat4 get_view_matrix() {
        glm::vec3 eye = get_eye();

        glm::vec3 forward = glm::normalize(glm::vec3(0.0f) - eye);
        glm::vec3 right = glm::normalize(glm::cross(forward, world_up));
        glm::vec3 cam_up = glm::normalize(glm::cross(right, forward));
        return glm::lookAt(eye, glm::vec3(0.0f), cam_up);
    }

};

#endif