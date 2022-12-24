#pragma once

#include "gltf/gltf.h"

class Animation_Player {
public:
  float previous_time = 0.0f;
  float current_time = 0.0f;
  float total_animation_duration = 0.0f;

private:
  bool paused = false;
  const Animation& animation;
  GLTF_Data& animation_data;

  void calculate_total_animation_duration() {
    // https://github.com/KhronosGroup/glTF/issues/1184
    // The total duration in seconds is just the max of all max values of input samplers within a given animation.
    double max = 0.0f;

    for(const auto& sampler : animation.samplers) {
      max = std::max(max, double(*std::max_element(animation_data.accessors[sampler.input].max.begin(), animation_data.accessors[sampler.input].max.end())));
    }

    total_animation_duration = max;
  }

public:

  Animation_Player(GLTF_Data& animation_data, const Animation& animation) : animation_data(animation_data), animation(animation) {
    calculate_total_animation_duration();
  }

  void pause() {
    paused = true;
  }

  void play(float time_in_seconds_to_increment_the_animation) {
    if(paused) return;

    previous_time = current_time;
    current_time += time_in_seconds_to_increment_the_animation;

    // Remove this later. Loop animation:
    if(current_time > total_animation_duration) {
      current_time = 0.0f;
      previous_time = 0.0f;
    }

    for(auto& channel : animation.channels) {
      auto& node = animation_data.nodes[channel.target_node];
      auto sampler = animation.samplers[channel.sampler];
      auto time_steps_accessor = animation_data.accessors[sampler.input];
      auto transform_accessor = animation_data.accessors[sampler.output];
      int num_of_components = tinygltf::GetNumComponentsInType(transform_accessor.type);
      auto time_steps_buffer = std::span(std::next(animation_data.gl_buffers[time_steps_accessor.buffer_view].data.begin(), time_steps_accessor.byte_offset), time_steps_accessor.count * sizeof(float));
      auto transform_buffer = std::span(std::next(animation_data.gl_buffers[transform_accessor.buffer_view].data.begin(), transform_accessor.byte_offset), transform_accessor.count * sizeof(float) * num_of_components);
      std::vector<float> time_steps;
      const auto* time_steps_buffer_f32 = reinterpret_cast<const float*>(time_steps_buffer.data());
      for (int i = 0; i < time_steps_accessor.count; ++i)
        time_steps.push_back(time_steps_buffer_f32[i]);
      std::vector<float> transform;
      const auto* transform_buffer_f32 = reinterpret_cast<const float*>(transform_buffer.data());
      for (int i = 0; i < transform_accessor.count * num_of_components; ++i)
        transform.push_back(transform_buffer_f32[i]);

      int lower_bound_time_index = 0;
      int upper_bound_time_index = 0;

      float gltf_previous_time{};
      float gltf_next_time{};

      for(int i = 0; i < time_steps.size() - 1; ++i) {
        float time = time_steps[i];
        gltf_previous_time = time;
        gltf_next_time = time_steps[i + 1];

        lower_bound_time_index = i;
        upper_bound_time_index = i + 1;

        // TODO: Determine if we need this...
      /*  if(current_time <= time) {
          if(current_time > time_steps[i+1]) {
            continue;
          }
          break;
        }*/

      }

      float interpolation = (current_time - gltf_previous_time) / (gltf_next_time - gltf_previous_time);

      if(channel.target_path == Target_Path::Rotation) {
        glm::quat lower(transform[lower_bound_time_index * num_of_components + 3], transform[lower_bound_time_index * num_of_components + 0], transform[lower_bound_time_index * num_of_components + 1], transform[lower_bound_time_index * num_of_components + 2]);
        glm::quat upper(transform[upper_bound_time_index * num_of_components + 3], transform[upper_bound_time_index * num_of_components + 0], transform[upper_bound_time_index * num_of_components + 1], transform[upper_bound_time_index * num_of_components + 2]);

        node.animation_transform = glm::mat4_cast(glm::slerp(lower, upper, interpolation));
      }

    }
  }

  void loop() {

  }

};