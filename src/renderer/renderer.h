#pragma once

#include <glad/glad.h>
#include <stb_image.h>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <functional>
#include "glm/gtc/type_ptr.hpp"
#include <filesystem>

struct Shader_Program {
  unsigned int renderer_id;

  Shader_Program(const std::string& vertexShaderString, const std::string& fragmentShaderString) {
    renderer_id = glCreateProgram();

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    const char* vertexShaderStringTemp = vertexShaderString.c_str();
    const char* fragmentShaderStringTemp = fragmentShaderString.c_str();

    glShaderSource(vertexShader, 1, &vertexShaderStringTemp, nullptr);
    glShaderSource(fragmentShader, 1, &fragmentShaderStringTemp, nullptr);

    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);

    glAttachShader(renderer_id, vertexShader);
    glAttachShader(renderer_id, fragmentShader);
    glLinkProgram(renderer_id);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
  }

  void set_int(const std::string& uniformName, int uniform_data) const {
    glUniform1i(glGetUniformLocation(renderer_id, uniformName.c_str()), uniform_data);
  }
  void set_mat4(const std::string& uniformName, const glm::mat4& uniform_data, bool transpose = false) const {
    glUniformMatrix4fv(glGetUniformLocation(renderer_id, uniformName.c_str()), 1, transpose, glm::value_ptr(uniform_data));
  }

  void bind() const {
    glUseProgram(renderer_id);
  }

  void unbind() const {
    glUseProgram(0);
  }

  ~Shader_Program() {
    glDeleteProgram(renderer_id);
  }
};

