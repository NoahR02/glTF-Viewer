#pragma once
#include "../renderer.h"
#include "../gl.h"
#include "common.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_gltf.h>
#include <span>

namespace gltf {
  struct Data {

    Scene_Handle default_scene = Invalid_Scene_Handle;
    // All Scenes, Nodes, and Meshes.
    // NOTE: Scenes can share nodes.
    std::vector<Scene> scenes{};
    std::vector<Node> nodes{};
    std::vector<Mesh> meshes{};
    std::vector<Accessor> accessors{};
    std::vector<Material> materials{};
    std::vector<Texture2D> textures{};
    std::vector<Animation> animations{};
    std::vector<Buffer_View> buffer_views{};

    Material default_material{};

    // NOTE: The indices map to glTF buffer view indices, not glTF buffers!
    // gl_buffers[0] -> cgltf_data.buffer_views[0]
    std::vector<Buffer> gl_buffers{};

    Data() {
      glGenTextures(1, reinterpret_cast<GLuint *>(&default_material.base_texture));
      glBindTexture(GL_TEXTURE_2D, default_material.base_texture);
      glActiveTexture(0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      unsigned char white_texture[4] = {255, 255, 255, 255};

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white_texture[0]);
    }

  private:

    void load_scenes(tinygltf::Model& gltf_data) {
      for (int scene_index = 0; scene_index < gltf_data.scenes.size(); ++scene_index) {
        auto& gltf_scene = gltf_data.scenes[scene_index];
        auto& scene = scenes[scene_index];

        scene.name = std::move(gltf_scene.name);
        scene.nodes = std::move(gltf_scene.nodes);
      }
    }

    void load_buffer_views(const tinygltf::Model& gltf_data) {
      for (int buffer_view_index = 0; buffer_view_index < gltf_data.bufferViews.size(); ++buffer_view_index) {
        auto& gltf_buffer_view = gltf_data.bufferViews[buffer_view_index];
        auto& buffer_view = buffer_views[buffer_view_index];

        buffer_view.byte_offset = gltf_buffer_view.byteOffset;
        buffer_view.byte_length = gltf_buffer_view.byteLength;
        buffer_view.byte_stride = gltf_buffer_view.byteStride;
        buffer_view.buffer = gltf_buffer_view.buffer;
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

    void load_accessors(tinygltf::Model& gltf_data) {
      // Get all the data we need to configure the Vertex Attributes later on.
      for(int accessor_index = 0; accessor_index < gltf_data.accessors.size(); ++accessor_index) {
        auto& gltf_accessor = gltf_data.accessors[accessor_index];
        auto& accessor = accessors[accessor_index];

        accessor.buffer_view = gltf_accessor.bufferView;
        accessor.byte_offset = gltf_accessor.byteOffset;
        accessor.component_type = gltf_accessor.componentType;
        accessor.type = gltf_accessor.type;
        accessor.count = gltf_accessor.count;

        accessor.min = std::move(gltf_accessor.minValues);
        accessor.max = std::move(gltf_accessor.maxValues);
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

    void load_mesh(tinygltf::Model& gltf_data) {
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
            if (gltf_attribute.first == "POSITION") slot = 0;
            if (gltf_attribute.first == "NORMAL") slot = 1;
            if (gltf_attribute.first == "TEXCOORD_0") slot = 2;
            if (gltf_attribute.first == "JOINTS_0") slot = 3;
            if (gltf_attribute.first == "WEIGHTS_0") slot = 4;
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


    void load_nodes(tinygltf::Model& gltf_data) {
      for(int node_index = 0; node_index < gltf_data.nodes.size(); ++node_index) {
        auto& gltf_node = gltf_data.nodes[node_index];
        auto& node = nodes[node_index];

        node.name = std::move(gltf_node.name);
        node.children = std::move(gltf_node.children);
        node.mesh = gltf_node.mesh;

        glm::mat4 translation_mat4(1.0f);
        glm::mat4 rotation_mat4(1.0f);
        glm::mat4 scale_mat4(1.0f);
        glm::mat4 gltf_matrix(1.0f);

        if(!gltf_node.translation.empty()) {
          translation_mat4[3][0] = gltf_node.translation[0];
          translation_mat4[3][1] = gltf_node.translation[1];
          translation_mat4[3][2] = gltf_node.translation[2];
        }

        if(!gltf_node.rotation.empty()) {
          // glTF Quat = (x, y, z, w)
          // glm Quat constructor = (w, x, y, z)
          glm::quat tmp_quat(gltf_node.rotation[3], gltf_node.rotation[0], gltf_node.rotation[1], gltf_node.rotation[2]);
          rotation_mat4 = glm::mat4_cast(tmp_quat);
        }

        if(!gltf_node.scale.empty()) {
          scale_mat4[0][0] = gltf_node.scale[0];
          scale_mat4[1][1] = gltf_node.scale[1];
          scale_mat4[2][2] = gltf_node.scale[2];
        }

        if(!gltf_node.matrix.empty()) {
          gltf_matrix = glm::make_mat4(&gltf_node.matrix[0]);

          glm::quat rot{};
          glm::vec3 scale{}, skew{}, translation{};
          glm::vec4 perpsective;
          glm::decompose(gltf_matrix, scale, rot, translation, skew, perpsective);

          translation_mat4[3][0] = translation[0];
          translation_mat4[3][1] = translation[1];
          translation_mat4[3][2] = translation[2];
          rotation_mat4 = glm::mat4_cast(rot);
          scale_mat4[0][0] = scale[0];
          scale_mat4[1][1] = scale[1];
          scale_mat4[2][2] = scale[2];
        }


        node.translation = translation_mat4;
        node.rotation = rotation_mat4;
        node.scale = scale_mat4;


        if(node.mesh != Invalid_Mesh_Handle) load_mesh(gltf_data);
      }
    }

    void load_animations(tinygltf::Model& gltf_data) {

      for (int animation_index = 0; animation_index < gltf_data.animations.size(); ++animation_index) {
        auto& gltf_animation = gltf_data.animations[animation_index];
        auto& animation = animations[animation_index];

        animation.channels.resize(gltf_animation.channels.size());

        //animation.name = std::move(gltf_animation.name);

        for(int channel_index = 0; channel_index < gltf_animation.channels.size(); ++channel_index) {
          auto& gltf_channel = gltf_animation.channels[channel_index];
          auto& channel = animation.channels[channel_index];

          if (gltf_channel.target_path == "translation") channel.target_path = Target_Path::Translation;
          if (gltf_channel.target_path == "rotation") channel.target_path = Target_Path::Rotation;
          if (gltf_channel.target_path == "scale") channel.target_path = Target_Path::Scale;
          if (gltf_channel.target_path == "weights") channel.target_path = Target_Path::Weights;

          channel.target_node = Node_Handle(gltf_channel.target_node);
          auto& gltf_sampler = gltf_animation.samplers[gltf_channel.sampler];

          if(gltf_sampler.interpolation == "LINEAR") channel.interpolation = Interpolation::Linear;

          auto& input_buffer_accessor = accessors[gltf_sampler.input];
          auto& output_buffer_accessor = accessors[gltf_sampler.output];

          auto& gltf_input_buffer_accessor = gltf_data.accessors[gltf_sampler.input];
          auto& gltf_output_buffer_accessor = gltf_data.accessors[gltf_sampler.output];
          auto& gltf_input_buffer_bufferview= gltf_data.bufferViews[input_buffer_accessor.buffer_view];
          auto& gltf_output_buffer_bufferview = gltf_data.bufferViews[output_buffer_accessor.buffer_view];

          Buffer& input_buffer = gl_buffers[accessors[gltf_sampler.input].buffer_view];
          Buffer& output_buffer = gl_buffers[accessors[gltf_sampler.output].buffer_view];

          const auto gltf_input_buffer = gltf_data.buffers[gltf_data.bufferViews[gltf_input_buffer_accessor.bufferView].buffer];
          const auto gltf_output_buffer = gltf_data.buffers[gltf_data.bufferViews[gltf_output_buffer_accessor.bufferView].buffer];

          input_buffer.data.resize(gltf_data.bufferViews[gltf_input_buffer_accessor.bufferView].byteLength);
          output_buffer.data.resize(gltf_data.bufferViews[gltf_output_buffer_accessor.bufferView].byteLength);

          int num_of_components = tinygltf::GetNumComponentsInType(gltf_output_buffer_accessor.type);
          auto time_steps_buffer = std::span(std::next(gltf_input_buffer.data.begin(), gltf_input_buffer_bufferview.byteOffset + input_buffer_accessor.byte_offset), input_buffer_accessor.count * sizeof(float));
          auto transform_buffer = std::span(std::next(gltf_output_buffer.data.begin(), gltf_output_buffer_bufferview.byteOffset + output_buffer_accessor.byte_offset), output_buffer_accessor.count * sizeof(float) * num_of_components);
          const auto* time_steps_buffer_f32 = reinterpret_cast<const float*>(time_steps_buffer.data());
          const auto* transform_buffer_f32 = reinterpret_cast<const float*>(transform_buffer.data());

          for (int i = 0; i < input_buffer_accessor.count; ++i) {
            Frame frame;

            frame.time = time_steps_buffer_f32[i];

            if(channel.target_path == gltf::Target_Path::Translation) {
              frame.translation = glm::vec3(transform_buffer_f32[i * num_of_components + 0], transform_buffer_f32[i * num_of_components + 1], transform_buffer_f32[i * num_of_components + 2]);
            } else if(channel.target_path == gltf::Target_Path::Rotation) {
              frame.rotation = glm::quat(transform_buffer_f32[i * num_of_components + 3], transform_buffer_f32[i * num_of_components + 0], transform_buffer_f32[i * num_of_components + 1], transform_buffer_f32[i * num_of_components + 2]);
            }

            channel.frames.push_back(frame);
          }

          animation.total_animation_duration = std::max(animation.total_animation_duration, time_steps_buffer_f32[input_buffer_accessor.count - 1]);
          channel.start_time = channel.frames.front().time;
          channel.end_time = channel.frames.back().time;
        }

      }
    }

    void internal_gltf_load(tinygltf::Model& gltf_data) {
      scenes.resize(gltf_data.scenes.size());
      meshes.resize(gltf_data.meshes.size());
      nodes.resize(gltf_data.nodes.size());
      accessors.resize(gltf_data.accessors.size());
      gl_buffers.resize(gltf_data.bufferViews.size());
      buffer_views.resize(gltf_data.bufferViews.size());
      materials.resize(gltf_data.materials.size());
      textures.resize(gltf_data.textures.size());
      animations.resize(gltf_data.animations.size());

      load_buffer_views(gltf_data);
      load_accessors(gltf_data);
      load_textures(gltf_data);
      load_materials(gltf_data);
      load_nodes(gltf_data);
      load_scenes(gltf_data);
      load_animations(gltf_data);
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
    }

    float time = 0.0f;

    void draw_all_scenes(unsigned int shader) {
      std::function<void(const Node&, const glm::mat4&)> draw_node;
      draw_node = [&draw_node, this, &shader](const Node& node, const glm::mat4& transform) {

        if(node.mesh == Invalid_Mesh_Handle) {

        } else {
          auto mesh = meshes[node.mesh];

          for(auto sub_mesh : mesh.sub_meshes) { // Start
            glBindVertexArray(sub_mesh.vao.renderer_id);
            // TRS
            auto model = transform;

            Material material;
            if(sub_mesh.material == -1) {
              material = default_material;
            } else {
              material = materials[sub_mesh.material];
            }

            if(material.base_texture > -1) {
              Texture2D& base_texture = textures[material.base_texture];
              glBindTexture(GL_TEXTURE_2D, base_texture.renderer_id);
              glActiveTexture(0);

              glUniform1i(glGetUniformLocation(shader, "tex_slot"), 0);
            } else {
              glBindTexture(GL_TEXTURE_2D, material.base_texture);
              glActiveTexture(0);

              glUniform1i(glGetUniformLocation(shader, "tex_slot"), 0);
            }

            auto identity = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shader, "u_model"), 1, GL_FALSE, &model[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(shader, "u_animation_channel_1"), 1, GL_FALSE, &identity[0][0]);
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
          }

        } // End

        for(int child_node_handle : node.children) {
          auto child_node = nodes[child_node_handle];
          draw_node(child_node, transform * (child_node.translation * child_node.rotation * child_node.scale) * child_node.animation_transform);
        }

      };

      for(const auto& scene : scenes) {
        for(auto scene_node_handle : scene.nodes) {
          auto node = nodes[scene_node_handle];
          draw_node(node, (node.translation * node.rotation * node.scale) * node.animation_transform);
        }
      }

    }

  };
};