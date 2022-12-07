#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include "renderer/renderer.h"
#include "editor_camera.h"

#include <fstream>
#include "input.h"

#include "renderer/gltf.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <imgui/imgui.h>

std::string read_entire_file(const std::string& path) {
  std::ifstream file(path);
  assert(file.is_open());
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  return content;
}
Editor_Camera camera({0, 0, 3}, Projection_Data(1600.0f / 900));

void imgui_list_gltf_file(const GLTF_Data& gltf_data) {

  std::function<void(Node)> list_nodes_recursively;

  list_nodes_recursively = [gltf_data, &list_nodes_recursively] (const Node& node){
    const auto node_name = node.name.length() > 0 ? node.name.c_str() : "No Name";
    if(node.children.size() > 0) {
      if (ImGui::TreeNode(node_name)) {
        for (auto child_node_handle : node.children) {
          list_nodes_recursively(gltf_data.nodes[child_node_handle]);
        }
        ImGui::TreePop();
      }
    } else {
      ImGui::Text(node_name);
    }
  };

  for(const auto& scene : gltf_data.scenes) {

    const auto scene_name = scene.name.length() > 0 ? scene.name.c_str() : "No Name";
    if(ImGui::TreeNode(scene_name)) {
      for(const auto& node_handle : scene.nodes) {
        list_nodes_recursively(gltf_data.nodes[node_handle]);
      }
      ImGui::TreePop();
    }
  }

}

void displayLoop(GLFWwindow* window_handle) {
  stbi_set_flip_vertically_on_load(true);

  Shader_Program shader(read_entire_file("assets/basic.vs"), read_entire_file("assets/basic.fs"));

  glfwSetCursorPos(window_handle, 1600.0f / 2, 900.0f / 2);
  camera = Editor_Camera({0, 0, 3}, Projection_Data(1600.0f / 900));
  camera.projection_data.far = 1000.0f;

  float delta;
  float last_frame;

  GLTF_Data data;
  //data.load("assets/Box/glTF/Box.gltf");
  //data.load("assets/Sponza/glTF/Sponza.gltf");
  //data.load("assets/sasha/scene.gltf");
  data.load("assets/RiggedFigure/glTF/RiggedFigure.gltf");
  //data.load("assets/ABeautifulGame/glTF/ABeautifulGame.gltf");

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForOpenGL(window_handle, true);
  ImGui_ImplOpenGL3_Init("#version 450 core");
  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  glfwSetInputMode(window_handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  while (!glfwWindowShouldClose(window_handle)) {
    float now = glfwGetTime();
    delta = now - last_frame;
    last_frame = now;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    camera.process_input(delta);
    if(Input::mouse_cursor_changed) {
      camera.process_mouse_input();
    }

    if(Input::is_key_pressed(Key::KEY_TAB)) {
      auto cursor_state = glfwGetInputMode(window_handle, GLFW_CURSOR);
      if(cursor_state == GLFW_CURSOR_DISABLED) {
        glfwSetInputMode(window_handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      } else {
        glfwSetInputMode(window_handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      }
    }

    if(Input::is_key_pressed(Key::KEY_ESCAPE)) {
      break;
    }

    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    shader.bind();
    shader.set_mat4("u_projection", camera.projection);
    shader.set_mat4("u_view", camera.view);

    data.draw_all_scenes(shader.renderer_id);

    ImGui::Begin("GLTF File");
    imgui_list_gltf_file(data);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window_handle);
    Input::mouse_cursor_changed = false;
    glfwPollEvents();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

}

int main() {
  if (!glfwInit()) return -1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  auto window_handle = glfwCreateWindow(1600, 900, "Game", nullptr, nullptr);
  glfwMakeContextCurrent(window_handle);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  glfwSetKeyCallback(window_handle, key_callback);
  //glfwSetMouseButtonCallback(window_handle, mouse_callback);
  glfwSetCursorPosCallback(window_handle, cursor_pos_callback);

  glfwSwapInterval(0);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  displayLoop(window_handle);

  glfwTerminate();
  return 0;
}
