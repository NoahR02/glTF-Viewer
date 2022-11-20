package camera

import "core:math/linalg/glsl"
import "vendor:glfw"
import "../../input"

@private
first_mouse := true

Camera :: struct {
  view, projection: glsl.mat4,

  yaw, pitch: f32,

  world_up: glsl.vec3,
  position: glsl.vec3,
  up, right, direction: glsl.vec3,

  near, far: f32,
  aspect_ratio: f32,
  fov: f32,
}

init_camera :: proc(aspect_ratio: f32, fov: f32 = 45.0, position := glsl.vec3{}, near: f32 = 0.1, far: f32 = 100.0) -> (camera: Camera) {
  camera.yaw = -90
  camera.world_up = {0, 1, 0}
  camera.position = position
  camera.fov = fov
  camera.aspect_ratio = aspect_ratio
  camera.near = near
  camera.far = far

  recalculate_camera(&camera)
  return
}

process_input :: proc(using camera: ^Camera, delta: f64) {
  delta := f32(delta)
  cameraSpeed := 1.0 * delta;
  if input.is_key_pressed(glfw.KEY_LEFT_SHIFT) {
    cameraSpeed *= 10
  }
  if input.is_key_pressed(glfw.KEY_SPACE) do position += cameraSpeed * up;
  if input.is_key_pressed(glfw.KEY_LEFT_CONTROL) do position -= cameraSpeed * up

  if input.is_key_pressed(glfw.KEY_W) do position += cameraSpeed * direction
  if input.is_key_pressed(glfw.KEY_A) do position -= cameraSpeed * right
  if input.is_key_pressed(glfw.KEY_S) do position -= cameraSpeed * direction
  if input.is_key_pressed(glfw.KEY_D) do position += cameraSpeed * right


  recalculate_camera(camera)
}

process_mouse_input :: proc(using camera: ^Camera) {
  previous_x := input.prev_mouse_x()
  previous_y := input.prev_mouse_y()

  if first_mouse {
    previous_x = input.mouse_x()
    previous_y = input.mouse_y()
    first_mouse = false
  }
  dx := input.mouse_x() - previous_x
  dy := previous_y - input.mouse_y()

  sensitivity := 0.05
  dx *= sensitivity
  dy *= sensitivity

  yaw += f32(dx)
  pitch += f32(dy)

  // Make sure the camera doesn't flip.
  if(pitch > 89.0) do pitch = 89.0
  if(pitch < -89.0) do pitch = -89.0

  recalculate_camera(camera);
}

recalculate_camera :: proc(using camera: ^Camera) {
  direction.x = glsl.cos(glsl.radians(yaw)) * glsl.cos(glsl.radians(pitch));
  direction.y = glsl.sin(glsl.radians(pitch));
  direction.z = glsl.sin(glsl.radians(yaw)) * glsl.cos(glsl.radians(pitch));
  direction = glsl.normalize(direction);

  right = glsl.normalize(glsl.cross(direction, world_up));
  up = glsl.normalize(glsl.cross(right, direction));

  view = glsl.mat4LookAt(position, position + direction, up);
  projection = glsl.mat4Perspective(fov, aspect_ratio, near, far);
}