package renderer

import "../../third-party/cgltf"
import "core:strings"
import gl "vendor:OpenGL"
import "core:fmt"
import "core:slice"
import "core:mem"
import "core:runtime"

// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html

Node_Handle             :: distinct int
Scene_Handle            :: distinct int
Mesh_Handle             :: distinct int
Buffer_View_Handle      :: distinct int
Attribute_Layout_Handle :: distinct int

Invalid_Node_Handle             :: Node_Handle(-1)
Invalid_Scene_Handle            :: Scene_Handle(-1)
Invalid_Mesh_Handle             :: Mesh_Handle(-1)
Invalid_Buffer_View_Handle      :: Buffer_View_Handle(-1)
Invalid_Attribute_Layout_Handle :: Attribute_Layout_Handle(-1)

Accessor :: struct {
  buffer: Buffer_View_Handle,
  vertex_array: Vertex_Array,
}

// The attribute layout for VAOs
// https://docs.gl/gl3/glVertexAttribPointer
Attribute_Layout :: struct {
  buffer: Buffer_View_Handle,

  component_size: int,  // vao size
  component_type: Component_Type, // vao type
  vector_type: Vector_Type,
  stride: int,          // vao stride
  offset: int,          // vao pointer

  count: int,
}

// Each Mesh is NOT a draw call.
// Meshes have RenderObjects and each RenderObject IS a draw call.
Mesh :: struct {
  render_objects: [dynamic]Vertex_Array,
}

Node :: struct {
  mesh: Mesh_Handle,
  children: [dynamic]Node_Handle,
}

Scene :: struct {
  nodes: [dynamic]Node_Handle,
}

GLTF_Data :: struct {
  default_scene: Scene_Handle,
  // All Scenes, Nodes, and Meshes.
  // NOTE: Scenes can share nodes.
  scenes: [dynamic]Scene,
  nodes: [dynamic]Node,
  meshes: [dynamic]Mesh,
  attribute_layouts: [dynamic]Attribute_Layout,

  // NOTE: The indices map to glTF buffer view indices, not glTF buffers!
  // gl_buffers[0] -> cgltf_data.buffer_views[0]
  gl_buffers: [dynamic]Buffer,
}

gltf_load :: proc(path: string) -> (gltf_data: GLTF_Data) {

  options: cgltf.options
  data: ^cgltf.data
  path_cstr := strings.clone_to_cstring(path, context.temp_allocator)
  did_parse := cgltf.parse_file(&options, path_cstr, &data)
  did_load_buffers := cgltf.load_buffers(&options, data, path_cstr)
  
  if did_parse != .success {
    fmt.println("FAILED TO LOAD", path, "ERROR:", did_parse)
  }

  if did_load_buffers != .success {
    fmt.println("FAILED TO LOAD BUFFERS", path, "ERROR:", did_load_buffers)
  }

  internal_gltf_load(&gltf_data, data^)
  return
}

@(private)
internal_gltf_load :: proc(gltf_data: ^GLTF_Data, cgltf_data: cgltf.data) {
  
  load_buffers :: proc(gltf_data: ^GLTF_Data, cgltf_data: cgltf.data) {
    dummy_vao: u32
    gl.GenVertexArrays(1, &dummy_vao)
    gl.BindVertexArray(dummy_vao)
    defer gl.BindVertexArray(0)
    defer gl.DeleteVertexArrays(1, &dummy_vao)


    // For each buffer view, create an OpenGL Buffer.
    for buffer_view_index in 0..<cgltf_data.buffer_views_count {
      buffer_view := cgltf_data.buffer_views[buffer_view_index]
      
      buffer: Buffer
      defer append_elem(&gltf_data.gl_buffers, buffer)
      switch buffer_view.type {
        case .vertices: buffer.type = .Array_Buffer
        case .indices: buffer.type = .Element_Array_Buffer
        case .invalid: fmt.println("FIXME: Handle invalid buffer views.")
      }
      gl.GenBuffers(1, &buffer.renderer_id)
      gl.BindBuffer(u32(buffer.type), buffer.renderer_id)

      raw_buffer := slice.bytes_from_ptr(buffer_view.buffer.data, int(buffer_view.buffer.size))

      buffer_view_buffer := raw_buffer[buffer_view.offset: buffer_view.offset + buffer_view.size]
      gl.BufferData(u32(buffer.type), int(buffer_view.size), &buffer_view_buffer[0], gl.STATIC_DRAW)
    }
  }

  load_attribute_layouts :: proc(gltf_data: ^GLTF_Data, cgltf_data: cgltf.data) {
    
    // Get all the data we need to configure the Vertex Attributes later on.
    for accessor_index in 0..<cgltf_data.accessors_count  {
      accessor := cgltf_data.accessors[accessor_index]

      attribute_layout: Attribute_Layout
      defer append_elem(&gltf_data.attribute_layouts, attribute_layout)

      attribute_layout.buffer = find_cgltf_buffer_view_index(accessor.buffer_view, cgltf_data)
      attribute_layout.offset = int(accessor.offset)
      attribute_layout.component_type = cgltf_component_type_to_component_type(accessor.component_type)
      attribute_layout.vector_type = cgltf_vector_type_to_vector_type(accessor.type)
      attribute_layout.stride = int(accessor.stride)
      attribute_layout.count = int(accessor.count)
    }
    
  }

  load_meshes :: proc(gltf_data: ^GLTF_Data, cgltf_data: cgltf.data) {
    
    for mesh_index in 0..<cgltf_data.meshes_count {
      gltf_mesh := cgltf_data.meshes[mesh_index]
      mesh: Mesh
      defer append_elem(&gltf_data.meshes, mesh)
      
      for primitive_index in 0..<gltf_mesh.primitives_count {
        gltf_primitive := gltf_mesh.primitives[primitive_index]
        vao: Vertex_Array
        defer append_elem(&mesh.render_objects, vao)
         
        if gltf_primitive.indices == nil {
          fmt.eprintln("FIXME: Handle non indexed geometry")
          continue
        }

        gl.GenVertexArrays(1, &vao.renderer_id)
        gl.BindVertexArray(vao.renderer_id)

        for gltf_attribute_index in 0..<gltf_primitive.attributes_count {
          gltf_attribute := gltf_primitive.attributes[gltf_attribute_index]
          
          slot := 0
          switch gltf_attribute.name {
            case "POSITION": slot = 0
            case "NORMAL":   slot = 1
            case: fmt.println("FIXME: Add more attribute slots", gltf_attribute.name)
          }

          attribute_layout := gltf_data.attribute_layouts[find_cgltf_accessor_index(gltf_attribute.data, cgltf_data)]
          buffer := gltf_data.gl_buffers[attribute_layout.buffer]
          gl.BindBuffer(gl.ARRAY_BUFFER, buffer.renderer_id)
          
          gl.EnableVertexAttribArray(u32(slot))
          gl.VertexAttribPointer(u32(slot),
           i32(vector_type_to_number_of_components(attribute_layout.vector_type)),
           u32(attribute_layout.component_type),
           false,
           i32(attribute_layout.stride),
           uintptr(attribute_layout.offset))
        }

        indices_accessor := gltf_data.attribute_layouts[find_cgltf_accessor_index(gltf_primitive.indices, cgltf_data)]
        indices_buffer := gltf_data.gl_buffers[indices_accessor.buffer]
        gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, indices_buffer.renderer_id)
        gl.VertexArrayElementBuffer(vao.renderer_id, indices_buffer.renderer_id)
        vao.indices_component_type = indices_accessor.component_type
        vao.primitive_mode = cgltf_primitive_type_to_primitive_type(gltf_primitive.type)
        vao.count = indices_accessor.count
      }
    }

  }

  load_nodes :: proc(gltf_data: ^GLTF_Data, cgltf_data: cgltf.data) {
    
    for node_index in 0..<cgltf_data.nodes_count {
      gltf_node := cgltf_data.nodes[node_index]
      node: Node
      defer append_elem(&gltf_data.nodes, node)

      node.mesh = find_cgltf_mesh_index(gltf_node.mesh, cgltf_data)
      for child_node_index in 0..<gltf_node.children_count {
        child_node := find_cgltf_node_index(&cgltf_data.nodes[child_node_index], cgltf_data)
        append_elem(&node.children, child_node)
      }
      
    }

  }


  load_scenes :: proc(gltf_data: ^GLTF_Data, cgltf_data: cgltf.data) {
    for scene_index in 0..<cgltf_data.scenes_count {
      gltf_scene := cgltf_data.scenes[scene_index]
      scene: Scene
      defer append_elem(&gltf_data.scenes, scene)

      for node_index in 0..<gltf_scene.nodes_count {
        node := find_cgltf_node_index(&cgltf_data.nodes[node_index], cgltf_data)
        append_elem(&scene.nodes, node)
      }
      
    }
  }

  reserve_dynamic_array(&gltf_data.scenes, int(cgltf_data.scenes_count))
  reserve_dynamic_array(&gltf_data.meshes, int(cgltf_data.meshes_count))
  reserve_dynamic_array(&gltf_data.nodes, int(cgltf_data.nodes_count))
  reserve_dynamic_array(&gltf_data.attribute_layouts, int(cgltf_data.accessors_count))
  reserve_dynamic_array(&gltf_data.gl_buffers, int(cgltf_data.buffer_views_count))

  load_buffers(gltf_data, cgltf_data)
  load_attribute_layouts(gltf_data, cgltf_data)
  load_meshes(gltf_data, cgltf_data)
  load_nodes(gltf_data, cgltf_data)
  load_scenes(gltf_data, cgltf_data)
  gltf_data.default_scene = find_cgltf_scene_index(cgltf_data.scene, cgltf_data)
}

gltf_draw_all_scenes :: proc(gltf_data: GLTF_Data) {
  for mesh in gltf_data.meshes {
    for render_object in mesh.render_objects {
     gl.BindVertexArray(render_object.renderer_id)
     gl.DrawElements(u32(render_object.primitive_mode), i32(render_object.count), u32(render_object.indices_component_type), nil)
    }
  }
}