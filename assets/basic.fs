#version 450 core

layout(location = 0) out vec4 color;

uniform vec4 u_base_color;
uniform sampler2D tex_slot;
in vec2 in_tex_coords;

void main() {
	color = texture(tex_slot, in_tex_coords) * u_base_color;
}