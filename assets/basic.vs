#version 450 core

layout(location = 0) in vec3 xyz;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 tex_coords;

uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat4 u_model;

out vec2 in_tex_coords;

void main() {
	gl_Position = u_projection * u_view * u_model * vec4(xyz, 1.0);
	in_tex_coords = vec2(tex_coords.x, 1.0 - tex_coords.y);
}