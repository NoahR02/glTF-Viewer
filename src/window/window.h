#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>

#include "../input.h"

#include "./active_window.h"

enum class Window_Error {

};

class Window {

  int width{}, height{};
  std::string title{};
  GLFWwindow* window_handle;

public:

  void set_vsync(bool use_vsync) {
    glfwSwapInterval((int)use_vsync);
  }

  void set_width(int width) {
    this->width = width;
    glfwSetWindowSize(window_handle, this->width, height);
  }

  void set_height(int height) {
    this->height = height;
    glfwSetWindowSize(window_handle, width, this->height);
  }

  void set_size(int width, int height) {
    this->width = width;
    this->height = height;
    glfwSetWindowSize(window_handle, this->width, this->height);
  }

  void set_title(const std::string& title) {
    this->title = title;
    glfwSetWindowTitle(window_handle, this->title.c_str());
  }

  GLFWwindow* get_window_handle() {
    return window_handle;
  }

  Window_Error init(int width = 1600, int height = 900, const std::string& title = "Yuki Engine", int gl_major_version = 4, int gl_minor_version = 5)
  {
    this->width = width;
    this->height = height;
    this->title = title;
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, gl_major_version);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, gl_minor_version);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_handle = glfwCreateWindow(this->width, this->height, this->title.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(window_handle);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    switch_to_rendering_on_this_window();

    set_vsync(true);

    return {};
  }

  void switch_to_rendering_on_this_window() {
    active_window_handle = window_handle;
  }

  bool should_close() {
    return glfwWindowShouldClose(window_handle);
  }

  void swap_buffers() {
    glfwSwapBuffers(window_handle);
  }

  void poll_for_events() {
    glfwPollEvents();
  }

  void destroy() {
    glfwDestroyWindow(window_handle);
    glfwTerminate();
  }

};