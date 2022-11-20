package input

import "vendor:glfw"
import "../window"

previous_mouse_x, previous_mouse_y: f64
current_mouse_x, current_mouse_y: f64

is_key_pressed :: proc(key: int) -> bool {
  key := i32(key)
  state := glfw.GetKey(cast(glfw.WindowHandle)window.active_window_handle, key)
  return bool(state == glfw.PRESS || state == glfw.REPEAT)
}

is_mouse_button_down :: proc(mouse_button: int) -> bool {
  mouse_button := i32(mouse_button)
  state := glfw.GetMouseButton(cast(glfw.WindowHandle)window.active_window_handle, mouse_button)
  return bool(state == glfw.PRESS)
}

mouse_pos :: proc() -> (f64, f64) {
  x, y := glfw.GetCursorPos(cast(glfw.WindowHandle)window.active_window_handle)
  
  previous_mouse_x = current_mouse_x
  current_mouse_x = x

  previous_mouse_y = current_mouse_y
  current_mouse_y = y

  return x, y
}

prev_mouse_x :: proc() -> f64 {
  return previous_mouse_x
}

prev_mouse_y :: proc() -> f64 {
  return previous_mouse_y
}

prev_mouse_pos :: proc() -> (f64, f64) {
  return previous_mouse_x, previous_mouse_y
}

mouse_x :: proc() -> f64 {
  x, _ := mouse_pos()
  return x
}

mouse_y :: proc() -> f64 {
  _, y := mouse_pos()
  return y
}