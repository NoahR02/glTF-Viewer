package rpg

import "core:fmt"

import gl "vendor:OpenGL"
import "core:math/linalg/glsl"
import "vendor:glfw"

import "window"
import "input"
import "nodes/camera"

main :: proc() {

  game_window, err := window.create(1600, 900, "rpg")
  window.switch_to_rendering_on_this_window(game_window)
  
  vertices := [?]f32 {
    -0.5,  0.5, 0.0,
    -0.5, -0.5, 0.0,
    0.5,  0.5, 0.0,
    0.5, -0.5, 0.0,
  }

  indices := [?]u32 {
    0, 1, 2,
    2, 1, 3,
  }

  vao, ebo, vbo: u32

  gl.GenVertexArrays(1, &vao)
  gl.GenBuffers(1, &vbo)
  gl.GenBuffers(1, &ebo)

  gl.BindVertexArray(vao)
  gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
  gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, ebo)

  gl.BufferData(gl.ARRAY_BUFFER, size_of(vertices), &vertices[0], gl.STATIC_DRAW)
  gl.BufferData(gl.ELEMENT_ARRAY_BUFFER, size_of(indices), &indices[0], gl.STATIC_DRAW)

  gl.EnableVertexAttribArray(0)
  gl.VertexAttribPointer(0, 3, gl.FLOAT, false, 0, 0)

  shader, shader_err := gl.load_shaders("assets/basic.vs", "assets/basic.fs")
  assert(bool(shader) == true, "Failed to load shader...")

  editor_camera := camera.init_camera(f32(window.active_window_width/window.active_window_height), 45.0, {0, 0, 3})  

  gl.Enable(gl.DEPTH_TEST);  
  glfw.SetInputMode(cast(glfw.WindowHandle)window.active_window_handle, glfw.CURSOR, glfw.CURSOR_DISABLED)

  gl.Enable(gl.BLEND);
  gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);


  delta: f64
  last_frame: f64

  for !window.should_close(game_window) {
    now := glfw.GetTime()
    delta = now - last_frame
    last_frame = now

    window.switch_to_rendering_on_this_window(game_window)
    window.poll_events()

    camera.process_mouse_input(&editor_camera)
    camera.process_input(&editor_camera, delta)
  
    gl.ClearColor(0.3, 0, 0.3, 1)
    gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)

    gl.UseProgram(shader)
    gl.UniformMatrix4fv(gl.GetUniformLocation(shader, "u_projection"), 1, gl.FALSE, &editor_camera.projection[0][0])
    gl.UniformMatrix4fv(gl.GetUniformLocation(shader, "u_view"), 1, gl.FALSE, &editor_camera.view[0][0])
    gl.DrawElements(gl.TRIANGLES, len(indices), gl.UNSIGNED_INT, nil)

    window.swap_buffers(game_window)
  }

}