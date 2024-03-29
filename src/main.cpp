#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include "renderer/renderer.h"
#include "editor_camera.h"

#include <fstream>
#include "input.h"

#include "renderer/gltf/gltf.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <imgui/imgui.h>

#include "window/window.h"
#include "renderer/animation_player.h"

std::string read_entire_file(const std::string& path) {
  std::ifstream file(path);
  assert(file.is_open());
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  return content;
}
Editor_Camera camera({0, 0, 3}, Projection_Data(1600.0f / 900));


void render() {
  Window window;
  window.init();

  stbi_set_flip_vertically_on_load(true);

  Shader_Program shader(read_entire_file("assets/basic.vs"), read_entire_file("assets/basic.fs"));

  camera = Editor_Camera({0, 0, 3}, Projection_Data(1600.0f / 900));
  camera.projection_data.far = 10000000.0f;

  float delta{};
  float last_frame{};

  std::unique_ptr<gltf::Data> data = std::make_unique<gltf::Data>();
  //data.load("assets/Sponza/glTF/Sponza.gltf");
  //data->load("assets/AnimatedCube/glTF/AnimatedCube.gltf");
  //data->load("assets/simple_animation.gltf");
  //data->load("assets/BoxAnimated/glTF/BoxAnimated.gltf");
  //data->load("assets/AnimatedMorphCube/glTF/AnimatedMorphCube.gltf");
  data->load("assets/simple_morph.gltf");
  //Animation_Player animation_player(data.operator*(), data->animations[0]);
  //Animation animation(data.operator*(), data->animations[0]);
  //data.load("assets/RiggedFigure/glTF/RiggedFigure.gltf");
  //data->load("assets/Fox/glTF/Fox.gltf");
  //data->load("assets/2CylinderEngine/glTF/2CylinderEngine.gltf");
  //data.load("assets/glTF/FlightHelmet.gltf");
  //data.load("assets/sasha/scene.gltf");

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  ImGui_ImplGlfw_InitForOpenGL(window.get_window_handle(), true);
  ImGui_ImplOpenGL3_Init("#version 450 core");
  ImGui::StyleColorsDark();

  glfwSetInputMode(window.get_window_handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  while (!window.should_close()) {
    float now = glfwGetTime();
    delta = now - last_frame;
    last_frame = now;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    camera.process_input(delta);
    camera.process_mouse_input();

    if(Input::is_key_pressed(Key::KEY_TAB)) {
      auto cursor_state = glfwGetInputMode(window.get_window_handle(), GLFW_CURSOR);
      if(cursor_state == GLFW_CURSOR_DISABLED) {
        glfwSetInputMode(window.get_window_handle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      } else {
        glfwSetInputMode(window.get_window_handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      }
    }

    if(Input::is_key_pressed(Key::KEY_ESCAPE)) break;

    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader.bind();
    shader.set_mat4("u_projection", camera.projection);
    shader.set_mat4("u_view", camera.view);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    //animation_player.play(delta);
    data->draw_all_scenes(shader.renderer_id);
    //data2.draw_all_scenes(shader.renderer_id);

   /* ImGui::Begin("GLTF File");
    ImGui::Text("Animation");

    int i = 0;
    for(const auto& track : animation.tracks) {
      ImGui::Text("Track %d", i);
      ImGui::Text("Track Duration: %f", track.duration);
      ImGui::Text("Track Start Time: %f", track.start_time);
      for(const auto& frame : track.frames) {
        ImGui::Text("  Time %f", frame.time);
      }
      ++i;
    }
    ImGui::End();*/

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    window.swap_buffers();
    window.poll_for_events();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  window.destroy();
}

int main() {
  render();
  return 0;
}
