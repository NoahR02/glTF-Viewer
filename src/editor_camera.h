#pragma once

#include "glm/vec3.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "input.h"

struct Projection_Data {
  explicit Projection_Data(float aspect_ratio) : aspect_ratio((float)aspect_ratio){}
  Projection_Data() = delete;
  float fov = 45.0f;
  float aspect_ratio{};
  float near = 0.1f;
  float far = 1000000000.0f;
};

struct Editor_Camera {

  glm::vec3 position{};
  glm::mat4 view{};
  glm::mat4 projection{};
  Projection_Data projection_data;

  float yaw = -90.0f;
  float pitch = 0.0f;

  glm::vec3 world_up = {0, 1, 0};

  glm::vec3 up = {0, 1, 0};
  glm::vec3 right;
  glm::vec3 direction = {0.0, 0.0, -1.0f};

  float mouse_previous_x;
  float mouse_previous_y;

  float mouse_x;
  float mouse_y;

  Editor_Camera(const glm::vec3& position, const Projection_Data& projection_data) : position(position), projection_data(projection_data) {
    recalculate_camera();
  }

  void process_input(float delta) {
    float cameraSpeed = 20.0f * delta;

    if (Input::is_key_down(Key::KEY_LEFT_SHIFT))
      cameraSpeed *= 10;
    if (Input::is_key_down(Key::KEY_SPACE))
      position += cameraSpeed * up;
    if (Input::is_key_down(Key::KEY_LEFT_CONTROL))
      position -= cameraSpeed * up;

    if (Input::is_key_down(Key::KEY_W))
      position += cameraSpeed * direction;

    if (Input::is_key_down(Key::KEY_A))
      position -= cameraSpeed * right;

    if (Input::is_key_down(Key::KEY_S))
      position -= cameraSpeed * direction;

    if (Input::is_key_down(Key::KEY_D))
      position += cameraSpeed * right;


    recalculate_camera();
  }

  void process_mouse_input() {
    mouse_previous_x = mouse_x;
    mouse_previous_y = mouse_y;
    mouse_x = Input::mouse_x();
    mouse_y = Input::mouse_y();

    if(first_mouse) {
      int window_width; int window_height;
      glfwGetWindowSize(active_window_handle, &window_width, &window_height);
      mouse_previous_x = window_width / 2.0f;
      mouse_previous_y = window_height / 2.0f;
      first_mouse = false;
    }

    float dx = mouse_x - mouse_previous_x;
    float dy = mouse_previous_y - mouse_y;

    const float sensitivity = 0.05f;
    dx *= sensitivity;
    dy *= sensitivity;

    yaw += dx;
    pitch += dy;

    // Make sure the camera doesn't flip.
    if(pitch > 89.0f)
      pitch = 89.0f;
    if(pitch < -89.0f)
      pitch = -89.0f;

    recalculate_camera();
  }

private:
  bool first_mouse = true;

  void recalculate_camera() {
    direction.x = glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
    direction.y = glm::sin(glm::radians(pitch));
    direction.z = glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
    direction = glm::normalize(direction);

    right = glm::normalize(glm::cross(direction, world_up));
    up = glm::normalize(glm::cross(right, direction));

    view = glm::lookAt(position, position + direction, up);
    projection = glm::perspective(projection_data.fov, projection_data.aspect_ratio, projection_data.near, projection_data.far);
  }
};