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

struct Texture {

  unsigned int renderer_id;
  std::string type;
  int width;
  int height;
  int channels;


  explicit Texture(const std::string& texturePath) {
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &channels, 4);

    glGenTextures(1, &renderer_id);
    glBindTexture(GL_TEXTURE_2D, renderer_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  void bind() const {
    glBindTexture(GL_TEXTURE_2D, renderer_id);
  }

  void unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  Texture() {
    glDeleteTextures(1, &renderer_id);
  }
};

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

