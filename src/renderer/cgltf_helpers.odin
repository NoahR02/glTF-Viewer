package renderer

import "../../third-party/cgltf"
import "core:fmt"

@(private)
find_cgltf_buffer_view_index :: proc(buffer_ptr: ^cgltf.buffer_view, cgltf_data: cgltf.data) -> Buffer_View_Handle {
  for cgltf_node_index in 0..<cgltf_data.buffer_views_count {
    if buffer_ptr == &cgltf_data.buffer_views[cgltf_node_index] {
      return Buffer_View_Handle(cgltf_node_index)
    }
  }
  return Invalid_Buffer_View_Handle
}
@(private)
find_cgltf_node_index :: proc(node_ptr: ^cgltf.node, cgltf_data: cgltf.data) -> Node_Handle {
  for cgltf_node_index in 0..<cgltf_data.nodes_count {
    if node_ptr == &cgltf_data.nodes[cgltf_node_index] {
      return Node_Handle(cgltf_node_index)
    }
  }
  return Invalid_Node_Handle
}
@(private)
find_cgltf_mesh_index :: proc(mesh_ptr: ^cgltf.mesh, cgltf_data: cgltf.data) -> Mesh_Handle {
  for cgltf_mesh_index in 0..<cgltf_data.meshes_count {
    if mesh_ptr == &cgltf_data.meshes[cgltf_mesh_index] {
      return Mesh_Handle(cgltf_mesh_index)
    }
  }
  return Invalid_Mesh_Handle
}
@(private)
find_cgltf_scene_index :: proc(scene_ptr: ^cgltf.scene, cgltf_data: cgltf.data) -> Scene_Handle {
  for cgltf_scene_index in 0..<cgltf_data.scenes_count {
    if scene_ptr == &cgltf_data.scenes[cgltf_scene_index] {
      return Scene_Handle(cgltf_scene_index)
    }
  }
  return Invalid_Scene_Handle
}
@(private)
find_cgltf_accessor_index :: proc(accessor_ptr: ^cgltf.accessor, cgltf_data: cgltf.data) -> Accessor_Handle {
  for cgltf_accessor_index in 0..<cgltf_data.accessors_count {
    if accessor_ptr == &cgltf_data.accessors[cgltf_accessor_index] {
      return Accessor_Handle(cgltf_accessor_index)
    }
  }
  return Invalid_Accessor_Handle
}

@(private)
cgltf_component_type_to_component_type :: proc(cgltf_componet_type: cgltf.component_type) -> (component_type: Component_Type) {
  switch cgltf_componet_type {
    case .r_8:   component_type = .Byte
    case .r_8u:  component_type = .Unsigned_Byte
    case .r_16:  component_type = .Short
    case .r_16u: component_type = .Unsigned_Short
    case .r_32u: component_type = .Unsigned_Int
    case .r_32f: component_type = .Float
    case .invalid: fmt.eprintln("FIXME: handle invalid component types")
  }
  return
}

@(private)
cgltf_vector_type_to_vector_type :: proc(cgltf_vector_type: cgltf.type) -> (vector_type: Vector_Type) {
  switch cgltf_vector_type {
    case .scalar: vector_type = .Scalar
    case .vec2: vector_type = .Vec2
    case .vec3: vector_type = .Vec3
    case .vec4: vector_type = .Vec4
    case .invalid, .mat2, .mat3, .mat4: fmt.eprintln("FIXME: Handle more component types")
  }
  return
}

@(private)
cgltf_primitive_type_to_primitive_type :: proc(cgltf_primitive_type: cgltf.primitive_type) -> (primitive_mode: Primitive_Mode) {
  switch cgltf_primitive_type {
    case .triangles: primitive_mode = .Triangles
    case .triangle_strip: primitive_mode = .Triangle_Strip
    case .triangle_fan: primitive_mode = .Triangle_Fan
    case .lines: primitive_mode = .Lines
    case .points: primitive_mode = .Points
    case .line_loop: primitive_mode = .Line_Loop
    case .line_strip: primitive_mode = .Line_Strip
  }
  return
}