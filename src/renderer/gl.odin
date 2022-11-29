package renderer

import gl "vendor:OpenGL"
import "core:fmt"
import "core:runtime"

// Basic structures to keep gl related data together.
// The intention is not to create a OpenGL wrapper.

Buffer_Type :: enum {
  Invalid = -1,
  Array_Buffer = gl.ARRAY_BUFFER,
  Element_Array_Buffer = gl.ELEMENT_ARRAY_BUFFER,
}

Buffer :: struct {
  renderer_id: u32,
  type: Buffer_Type,
}

Primitive_Mode :: enum {
  Triangles      = gl.TRIANGLES,
  Triangle_Strip = gl.TRIANGLE_STRIP,
  Triangle_Fan   = gl.TRIANGLE_FAN,

  Points = gl.POINTS,
  Lines  = gl.LINES,
  
  Line_Loop  = gl.LINE_LOOP,
  Line_Strip = gl.LINE_STRIP,
}

Vector_Type :: enum { 
  Scalar = 1, 
  Vec2 = 3, 
  Vec3 = 3, 
  Vec4 = 4, 
}

Component_Type :: enum {
  Byte = gl.BYTE, Unsigned_Byte = gl.UNSIGNED_BYTE,
  Unsigned_Short = gl.UNSIGNED_SHORT, Short = gl.SHORT,
  
  Unsigned_Int = gl.UNSIGNED_INT, Int = gl.INT,

  Float = gl.FLOAT,
  Double = gl.DOUBLE,
}

/* // The attribute layout for VAOs
// https://docs.gl/gl3/glVertexAttribPointer
Attribute_Layout :: struct {
  component_size: int,  // vao size
  vector_type:    Vector_Type,
  component_type: Component_Type, // vao type
  stride: int,          // vao stride
  offset: int,          // vao pointer
} */

Vertex_Array :: struct {
  renderer_id: u32,
  primitive_mode: Primitive_Mode,
  indices_component_type: Component_Type,
  // How many Indicies? len(indices)
  count: int,
  
  has_indices: bool,
}

component_type_to_byte_size :: proc(type: Component_Type) -> int {
  switch type {
    case .Byte,  .Unsigned_Byte:   return size_of(byte)
    case .Short, .Unsigned_Short:  return size_of(u16)
    case .Int, .Unsigned_Int:      return size_of(u32)
    case .Float:                   return size_of(f32)
    case .Double:                  return size_of(f64)
    case: return -1
  }
}

vector_type_to_number_of_components :: proc(type: Vector_Type) -> int {
  return int(type)
}

debug_proc_t :: proc "c" (source: u32, type: u32, id: u32, severity: u32, length: i32, message: cstring, userParam: rawptr) {
  context = runtime.default_context()
  //if severity != gl.DEBUG_SEVERITY_NOTIFICATION
  //fmt.println(message)
}
