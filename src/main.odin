package rpg

import "core:fmt"
import "core:path/filepath"
import "core:encoding/json"
import "core:encoding/base64"
import "core:math/linalg/glsl"

import gl "vendor:OpenGL"
import "vendor:glfw"

import "window"
import "input"
import "nodes/camera"
import "renderer"

main :: proc() {
  game_window, err := window.create(1600, 900, "rpg")
  window.switch_to_rendering_on_this_window(game_window)
  gl.Enable(gl.DEBUG_OUTPUT)
  gl.Enable(gl.DEBUG_OUTPUT_SYNCHRONOUS)
  gl.DebugMessageCallback(renderer.debug_proc_t, nil)
  gl.DebugMessageControl(gl.DEBUG_SOURCE_API, 
    gl.DEBUG_TYPE_ERROR, 
    gl.DEBUG_SEVERITY_HIGH,
    0, nil, gl.TRUE)

  shader, shader_err := gl.load_shaders("assets/basic.vs", "assets/basic.fs")
  assert(bool(shader) == true, "Failed to load shader...")

  editor_camera := camera.init_camera(f32(window.active_window_width/window.active_window_height), 45.0, {0, 0, 3})  

  gl.Enable(gl.DEPTH_TEST)
  glfw.SetInputMode(cast(glfw.WindowHandle)window.active_window_handle, glfw.CURSOR, glfw.CURSOR_DISABLED)

  gl.Enable(gl.BLEND);
  gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

  //gltf_data := renderer.gltf_load("assets/Box/glTF/Box.gltf")
  //gltf_data := renderer.gltf_load("assets/Avocado/glTF/Avocado.gltf")
  // Not Working
  gltf_data := renderer.gltf_load("assets/Fox/glTF/Fox.gltf")
  //gltf_data := renderer.gltf_load("assets/glTF/FlightHelmet.gltf")

  delta: f64
  last_frame: f64
  for !window.should_close(game_window) {
    now := glfw.GetTime()
    delta = now - last_frame
    last_frame = now

    window.switch_to_rendering_on_this_window(game_window)
    window.poll_events()

    if input.is_key_pressed(glfw.KEY_ESCAPE) do break
    camera.process_mouse_input(&editor_camera)
    camera.process_input(&editor_camera, delta)
    
    gl.ClearColor(0.3, 0, 0.3, 1)
    gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)
    gl.PolygonMode( gl.FRONT_AND_BACK, gl.LINE );
    gl.UseProgram(shader)
    gl.UniformMatrix4fv(gl.GetUniformLocation(shader, "u_projection"), 1, gl.FALSE, &editor_camera.projection[0][0])
    gl.UniformMatrix4fv(gl.GetUniformLocation(shader, "u_view"), 1, gl.FALSE, &editor_camera.view[0][0])

    renderer.gltf_draw_all_scenes(gltf_data, shader)

    window.swap_buffers(game_window)
  }

}