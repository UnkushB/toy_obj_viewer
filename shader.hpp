#ifndef SHADER_HPP
#define SHADER_HPP

#include <iostream>

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void checkShaderCompilation(unsigned int& shader) {
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "shader comp failed\n" << infoLog << std::endl;
    }
}

void checkShaderLinkage(unsigned int shaderProgram) {
    int success;
    char infoLog[512];
    glGetShaderiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "shader vertex link failed\n" << infoLog << std::endl;
    }
}

class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath);

    void use();
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setMat4(const std::string& name, glm::mat4 value) const;
    void setVec3(const std::string& name, glm::vec3 value) const;
    void setVec2(const std::string& name, glm::vec2 value) const;
};

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    vShaderFile.open(vertexPath);
    fShaderFile.open(fragmentPath);
    std::stringstream vShaderStream, fShaderStream;

    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();

    vShaderFile.close();
    fShaderFile.close();

    vertexCode = vShaderStream.str();
    fragmentCode = fShaderStream.str();


    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    unsigned int vId, fId;
    vId = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vId, 1, &vShaderCode, NULL);
    glCompileShader(vId);
    checkShaderCompilation(vId);

    fId = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fId, 1, &fShaderCode, NULL);
    glCompileShader(fId);
    checkShaderCompilation(fId);

    ID = glCreateProgram();
    glAttachShader(ID, vId);
    glAttachShader(ID, fId);
    glLinkProgram(ID);
    checkShaderLinkage(ID);

    glDeleteShader(vId);
    glDeleteShader(fId);

}

void Shader::use() {
    glUseProgram(ID);
}

void Shader::setBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}
void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setMat4(const std::string& name, glm::mat4 value) const
{
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, glm::vec3 value) const
{
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setVec2(const std::string& name, glm::vec2 value) const
{
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}

#endif