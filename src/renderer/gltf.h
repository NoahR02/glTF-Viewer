#pragma once
#include "renderer.h"
#include "gl.h"
#include "tiny_gltf.h"

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

// The attribute layout for VAOs
// https://docs.gl/gl3/glVertexAttribPointer
struct Accessor {
  Buffer_View_Handle buffer_view = Invalid_Buffer_View_Handle;
  int component_type{}; // vao type
  int type{};
  int byte_offset{}; // vao pointer
  int count{};
};

struct Transform {
  glm::quat rotation{};
  glm::vec3 scale = {1.0f, 1.0f, 1.0f};
  glm::vec3 translation{};
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
  Transform transform{};
};

struct Scene {
  std::string name{};
  std::vector<Node_Handle>nodes;
};

struct GLTF_Data {

  Scene_Handle default_scene = Invalid_Scene_Handle;
  // All Scenes, Nodes, and Meshes.
  // NOTE: Scenes can share nodes.
  std::vector<Scene> scenes{};
  std::vector<Node> nodes{};
  std::vector<Mesh> meshes{};
  std::vector<Accessor> accessors{};
  std::vector<Material> materials{};
  std::vector<Texture2D> textures{};

  Material default_material{};

  // NOTE: The indices map to glTF buffer view indices, not glTF buffers!
  // gl_buffers[0] -> cgltf_data.buffer_views[0]
  std::vector<Buffer> gl_buffers{};

private:

  void load_scenes(const tinygltf::Model& gltf_data) {
    for (int scene_index = 0; scene_index < gltf_data.scenes.size(); ++scene_index) {
      const auto& gltf_scene = gltf_data.scenes[scene_index];
      auto& scene = scenes[scene_index];

      scene.name = std::move(gltf_scene.name);
      scene.nodes = std::move(gltf_scene.nodes);
    }
  }

  void load_buffer(const tinygltf::Model& gltf_data, int buffer_view_handle) {

    // If we have already uploaded the buffer before just return.
    if(auto current_buffer = gl_buffers[buffer_view_handle]; gl_buffers[buffer_view_handle].renderer_id != 0) {
      glBindBuffer(current_buffer.target, current_buffer.renderer_id);
      return;
    }

    Buffer& buffer = gl_buffers[buffer_view_handle];
    const auto& gltf_buffer_view = gltf_data.bufferViews[buffer_view_handle];
    const auto& gltf_buffer = gltf_data.buffers[gltf_buffer_view.buffer];

    buffer.target = gltf_buffer_view.target;
    if(gltf_buffer_view.target == 0) {
      buffer.target = GL_ELEMENT_ARRAY_BUFFER;
    }

    glGenBuffers(1, &buffer.renderer_id);
    glBindBuffer(buffer.target, buffer.renderer_id);
    glBufferData(buffer.target, gltf_buffer_view.byteLength, &gltf_buffer.data.at(0) + gltf_buffer_view.byteOffset, GL_STATIC_DRAW);
  }

  void load_attribute_layouts(const tinygltf::Model& gltf_data) {
    // Get all the data we need to configure the Vertex Attributes later on.
    for(int accessor_index = 0; accessor_index < gltf_data.accessors.size(); ++accessor_index) {
      const auto& gltf_accessor = gltf_data.accessors[accessor_index];
      auto& accessor = accessors[accessor_index];

      accessor.buffer_view = gltf_accessor.bufferView;
      accessor.byte_offset = gltf_accessor.byteOffset;
      accessor.component_type = gltf_accessor.componentType;
      accessor.type = gltf_accessor.type;
      accessor.count = gltf_accessor.count;
    }

  }

  void load_textures(const tinygltf::Model& gltf_data) {
    for (int texture_index = 0; texture_index < gltf_data.textures.size(); ++texture_index) {
      auto& texture = textures[texture_index];
      const auto& gltf_texture = gltf_data.textures[texture_index];
      const auto& gltf_image = gltf_data.images[gltf_texture.source];

      glGenTextures(1, &texture.renderer_id);
      glBindTexture(GL_TEXTURE_2D, texture.renderer_id);
      glActiveTexture(0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      GLenum format{};
      switch (gltf_image.component) {
        case 1: { format = GL_RED;  break; }
        case 2: { format = GL_RG;   break; }
        case 3: { format = GL_RGB;  break; }
        case 4: { format = GL_RGBA; break; }
        default: {
          std::cout << "Unknown texture_format: " << format << std::endl;
        }
      }

      GLenum type{};
      switch (gltf_image.bits) {
        case 16: { type = GL_UNSIGNED_SHORT; break; }
        case 8:  { type = GL_UNSIGNED_BYTE;  break; }
        default: {
          std::cout << "Unknown texture size: " << gltf_image.bits << std::endl;
        }
      }

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gltf_image.width, gltf_image.height, 0, format, type, &gltf_image.image.at(0));

    }
  }

  void load_materials(const tinygltf::Model& gltf_data) {
    for (int material_index = 0; material_index < gltf_data.materials.size(); ++material_index) {
      auto& material = materials[material_index];
      const auto& gltf_material = gltf_data.materials[material_index];

      // Capture the base color
      for(int i = 0; i < gltf_material.pbrMetallicRoughness.baseColorFactor.size(); ++i) {
        std::vector base_color = gltf_material.pbrMetallicRoughness.baseColorFactor;
        material.base_color[i] = gltf_material.pbrMetallicRoughness.baseColorFactor[i];
      }
      // Capture the base texture
      material.base_texture = gltf_material.pbrMetallicRoughness.baseColorTexture.index;

    }
  }

  void load_mesh(const tinygltf::Model& gltf_data) {
    for (int mesh_index = 0; mesh_index < gltf_data.meshes.size(); ++mesh_index) {
      auto& mesh = meshes[mesh_index];
      const auto& gltf_mesh = gltf_data.meshes[mesh_index];

      for(int primitive_index = 0; primitive_index < gltf_mesh.primitives.size(); ++primitive_index) {
        const auto& gltf_primitive = gltf_mesh.primitives[primitive_index];
        Vertex_Array vao;
        glGenVertexArrays(1, &vao.renderer_id);
        glBindVertexArray(vao.renderer_id);

        auto accessor_draw_count = 0;
        for (auto& gltf_attribute : gltf_primitive.attributes) {
          auto slot = -1;
          if (gltf_attribute.first.compare("POSITION") == 0) slot = 0;
          if (gltf_attribute.first.compare("NORMAL") == 0) slot = 1;
          if (gltf_attribute.first.compare("TEXCOORD_0") == 0) slot = 2;
          if (gltf_attribute.first.compare("JOINTS_0") == 0) slot = 3;
          if (gltf_attribute.first.compare("WEIGHTS_0") == 0) slot = 4;
          if (slot <= -1) {
            continue;
          }

          auto accessors = this->accessors[gltf_attribute.second];
          load_buffer(gltf_data, gltf_data.accessors[gltf_attribute.second].bufferView);

          auto& gltf_accessor = gltf_data.accessors[gltf_attribute.second];
          int byte_stride = gltf_accessor.ByteStride(gltf_data.bufferViews[gltf_accessor.bufferView]);
          glEnableVertexAttribArray(slot);
          glVertexAttribPointer(slot,
            (tinygltf::GetNumComponentsInType(accessors.type)),
            (accessors.component_type),
           gltf_accessor.normalized ? GL_TRUE : GL_FALSE,
                                byte_stride,
                                BUFFER_OFFSET(gltf_accessor.byteOffset));
          accessor_draw_count = int(gltf_accessor.count);
        }

        if(gltf_primitive.indices <= -1) {
            vao.has_indices = false;
            // When we are not working with indexed geometry, then use the accessor count which should be the same for each attribute's accessor.
            vao.count = accessor_draw_count;
            vao.primitive_mode = static_cast<Primitive_Mode>((int) gltf_primitive.mode);
        } else {
          vao.has_indices = true;
          auto indices_accessor = accessors[gltf_primitive.indices];
          load_buffer(gltf_data, indices_accessor.buffer_view);
          auto indices_buffer = gl_buffers[indices_accessor.buffer_view];
          glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_buffer.renderer_id);
          vao.indices_component_type = static_cast<Component_Type>(indices_accessor.component_type);
          vao.primitive_mode = static_cast<Primitive_Mode>((int) gltf_primitive.mode);
          vao.count = indices_accessor.count;
          vao.offset = indices_accessor.byte_offset;
        }
        SubMesh sub_mesh;
        sub_mesh.vao = vao;
        sub_mesh.material = gltf_primitive.material;
        mesh.sub_meshes.push_back(sub_mesh);
      }

    }

  }


  void load_nodes(const tinygltf::Model& gltf_data) {
    for(int node_index = 0; node_index < gltf_data.nodes.size(); ++node_index) {
      const auto& gltf_node = gltf_data.nodes[node_index];
      auto& node = nodes[node_index];

      node.name = std::move(gltf_node.name);
      node.children = std::move(gltf_node.children);
      node.mesh = gltf_node.mesh;
      for(int i = 0; i < gltf_node.translation.size(); ++i) {
        node.transform.translation[i] = gltf_node.translation[i];
      }
      if(node.mesh != Invalid_Mesh_Handle) load_mesh(gltf_data);
    }
  }

  void internal_gltf_load(const tinygltf::Model& gltf_data) {
    scenes.resize(gltf_data.scenes.size());
    meshes.resize(gltf_data.meshes.size());
    nodes.resize(gltf_data.nodes.size());
    accessors.resize(gltf_data.accessors.size());
    gl_buffers.resize(gltf_data.bufferViews.size());
    materials.resize(gltf_data.materials.size());
    textures.resize(gltf_data.textures.size());

    load_attribute_layouts(gltf_data);
    load_textures(gltf_data);
    load_materials(gltf_data);
    load_nodes(gltf_data);
    load_scenes(gltf_data);
    //gltf_data.default_scene = find_cgltf_scene_index(cgltf_data.scene, cgltf_data)
  }

  public:
    void load(const std::string& path) {
      tinygltf::TinyGLTF loader;
      tinygltf::Model data;
      std::string err;
      std::string warn;

      bool res = loader.LoadASCIIFromFile(&data, &err, &warn, path);

      internal_gltf_load(data);
      return;
    }

  void draw_all_scenes(unsigned int shader) {

    for(auto scene : scenes) {
      for(auto node : nodes) {

        if(node.mesh == Invalid_Mesh_Handle) continue;
        auto mesh = meshes[node.mesh];

        for(auto sub_mesh : mesh.sub_meshes) { // Start
          glBindVertexArray(sub_mesh.vao.renderer_id);
          auto model = glm::translate(glm::mat4(1.0f), node.transform.translation);

          Material material;
          if(sub_mesh.material == -1) {
            material = default_material;
            std::cout << "???";
          } else {
            material = materials[sub_mesh.material];
          }

          Texture2D& base_texture = textures[material.base_texture];
          glBindTexture(GL_TEXTURE_2D, base_texture.renderer_id);
          glActiveTexture(0);

          glUniform1i(glGetUniformLocation(shader, "tex_slot"), 0);
          glUniformMatrix4fv(glGetUniformLocation(shader, "u_model"), 1, GL_FALSE, &model[0][0]);
          glUniform4fv(glGetUniformLocation(shader, "u_base_color"), 1, &material.base_color[0]);

          if(sub_mesh.vao.has_indices) {
            glDrawElements(
              static_cast<GLenum>(sub_mesh.vao.primitive_mode),
              sub_mesh.vao.count,
              static_cast<GLenum>(sub_mesh.vao.indices_component_type),
              // The byte offset FROM the start of the buffer view.
              reinterpret_cast<const void *>(uintptr_t(sub_mesh.vao.offset)));
          } else {
            glDrawArrays(static_cast<GLenum>(sub_mesh.vao.primitive_mode), 0, (sub_mesh.vao.count));
          }

        } // End

      }
    }

  }

};