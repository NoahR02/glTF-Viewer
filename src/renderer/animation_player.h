#pragma once

#include "gltf/gltf.h"

class Animation_Player {

  bool paused = false;
  const gltf::Animation& animation;
  gltf::Data& animation_data;

public:
  float current_time = 0.0f;
  Animation_Player(gltf::Data& animation_data, const gltf::Animation& animation) : animation_data(animation_data), animation(animation) {


  }

  void pause() {
    paused = true;
  }

  void play(float time_in_seconds_to_increment_the_animation) {
    if(paused) return;

    current_time += time_in_seconds_to_increment_the_animation;

    // Remove this later. Loop animation:
    if(current_time > animation.total_animation_duration) {
      current_time = 0.0f;
    }

    for(auto& channel : animation.channels) {
      auto& node = animation_data.nodes[channel.target_node];

      if(current_time < channel.start_time || current_time > channel.end_time) continue;

      int lower_bound_time_index = 0;
      int upper_bound_time_index = 0;

      float gltf_previous_time{};
      float gltf_next_time{};

      for(int i = 0; i < channel.frames.size() - 1; ++i) {
        float time = channel.frames[i].time;
        gltf_previous_time = time;
        gltf_next_time = channel.frames[i + 1].time;

        lower_bound_time_index = i;
        upper_bound_time_index = i + 1;

        if(current_time <= gltf_next_time) {
          break;
        }

      }

      float interpolation = (current_time - gltf_previous_time) / (gltf_next_time - gltf_previous_time);

      const auto& lower_frame = channel.frames[lower_bound_time_index];
      const auto& upper_frame = channel.frames[upper_bound_time_index];

      if(channel.target_path == gltf::Target_Path::Rotation) {
        node.animation_transform = glm::mat4_cast(glm::slerp(lower_frame.rotation, upper_frame.rotation, interpolation));
      } else if(channel.target_path == gltf::Target_Path::Translation) {
        std::cout << "Lower: " << lower_bound_time_index << ", Upper: " << upper_bound_time_index << " / " << channel.frames.size() << std::endl;
        auto translation = lower_frame.translation + interpolation * (upper_frame.translation - lower_frame.translation);

        // Translate it back to 0.
        //node.translation = glm::mat4(1.0f);
        auto current_animation_translation = glm::vec3(node.animation_transform[3]);
        node.animation_transform = glm::translate(node.animation_transform, -current_animation_translation);

        auto current_translation = glm::vec3(node.translation[3]);
        node.translation = glm::translate(node.translation, -current_translation);


        // Now apply our animation translation.
        node.animation_transform = glm::translate(node.animation_transform, translation);
      }

    }
  }

  void loop() {

  }

};