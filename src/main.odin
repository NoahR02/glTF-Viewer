package rpg

import "core:fmt"

import gl "vendor:OpenGL"
import "vendor:glfw"

import "window"

main :: proc() {

  game_window, err := window.create(1600, 800, "rpg")

  for !window.should_close(game_window) {
    window.poll_events()

    gl.ClearColor(1, 0, 0, 1)
    gl.Clear(gl.COLOR_BUFFER_BIT)

    window.swap_buffers(game_window)
  }

}