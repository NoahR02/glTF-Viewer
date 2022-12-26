#pragma once

#include <span>

namespace gltf {
  typedef int Node_Handle;
  typedef int Scene_Handle;
  typedef int Mesh_Handle;
  typedef int Buffer_View_Handle;
  typedef int Accessor_Handle;

  constexpr Node_Handle Invalid_Node_Handle = Node_Handle(-1);
  constexpr Node_Handle Invalid_Scene_Handle = Scene_Handle(-1);
  constexpr Node_Handle Invalid_Mesh_Handle = Mesh_Handle(-1);
  constexpr Node_Handle Invalid_Buffer_View_Handle = Buffer_View_Handle(-1);
  constexpr Node_Handle Invalid_Accessor_Handle = Accessor_Handle(-1);

  struct Accessor {
    Buffer_View_Handle buffer_view = Invalid_Buffer_View_Handle;
    int component_type{}; // vao type
    int type{}; // vector type(vec3, vec4, and etc)
    int byte_offset{}; // vao pointer
    int count{}; // number of vector types, 5 vec3s

    std::vector<double> min;
    std::vector<double> max;
  };

  struct Texture2D {
    unsigned int renderer_id{};
  };

  struct Material {
    glm::vec4 base_color = {1.0f, 1.0f, 1.0f, 1.0f};
    int base_texture = -1;
  };

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

  struct SubMesh {
    Vertex_Array vao{};
    int material{};
  };

// Each Mesh is NOT a draw call.
// Meshes have RenderObjects and each RenderObject IS a draw call.
  struct Mesh {
    std::string name{};
    std::vector<SubMesh> sub_meshes{};
  };

  struct Node {
    std::string name{};
    Mesh_Handle mesh = Invalid_Mesh_Handle;
    std::vector<Node_Handle> children{};
    glm::mat4 translation = glm::mat4(1.0f);
    glm::mat4 rotation = glm::mat4(1.0f);
    glm::mat4 scale = glm::mat4(1.0f);
    glm::mat4 animation_transform = glm::mat4(1.0f);
  };

  struct Buffer_View {
    int buffer = -1;
    int byte_offset{};
    int byte_length{};
    int byte_stride{};
    int target{};
  };

  struct Scene {
    std::string name{};
    std::vector<Node_Handle>nodes;
  };

  enum class Target_Path {
    Translation,
    Rotation,
    Scale,
    Weights,
  };

  enum class Interpolation {
    Linear,
  };

  struct Frame {
    float time{};
    glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
  };

  struct Animation_Channel {
    std::vector<Frame> frames;

    Node_Handle target_node = Invalid_Node_Handle;
    Target_Path target_path{};
    Interpolation interpolation{};

    float start_time{};
    float end_time{};
  };

  struct Animation {
    std::string name{};
    float total_animation_duration{};
    std::vector<Animation_Channel> channels;
  };

};

/*
std::span<unsigned char> get_buffer_view_slice(const Buffer& buffer, const Buffer_View& buffer_view) {
  return std::span<unsigned char>(std::next(buffer.data.begin(), buffer_view.byte_offset), buffer_view.byte_length);
}*/
