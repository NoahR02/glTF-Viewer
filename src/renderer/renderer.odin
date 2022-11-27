package renderer

import "../../third-party/cgltf"
import "core:strings"
import gl "vendor:OpenGL"
import "core:fmt"
import "core:slice"
import "core:mem"
import "core:runtime"



debug_proc_t :: proc "c" (source: u32, type: u32, id: u32, severity: u32, length: i32, message: cstring, userParam: rawptr) {
  context = runtime.default_context()
  //if severity != gl.DEBUG_SEVERITY_NOTIFICATION
  fmt.println(message)
}


// Model Loading START
// https://gltf-transform.donmccurdy.com/classes/core.primitive.html
// https://gist.github.com/seanbaxter/947417862033ff7870e564c7a79eb2d1#file-prim-cxx

Node_Handle   :: distinct int
Scene_Handle  :: distinct int
Mesh_Handle   :: distinct int
Buffer_Handle :: distinct int
Attribute_Layout_Handle :: distinct int

Invalid_Node_Handle   :: Node_Handle(-1)
Invalid_Scene_Handle  :: Scene_Handle(-1)
Invalid_Mesh_Handle   :: Mesh_Handle(-1)
Invalid_Buffer_Handle :: Buffer_Handle(-1)
Invalid_Attribute_Layout_Handle :: Attribute_Layout_Handle(-1)

GL_Buffer_Type :: enum {
  Invalid,
  ARRAY_BUFFER = 34962,
  ELEMENT_ARRAY_BUFFER = 34963,
}

GL_Buffer :: struct {
  buffer: u32,
  type: GL_Buffer_Type,
}

Attribute_Layout_Component_Type :: enum {
  BYTE = gl.BYTE,
  UNSIGNED_BYTE = gl.UNSIGNED_BYTE,
  
  SHORT = gl.SHORT,
  UNSIGNED_SHORT = gl.UNSIGNED_SHORT,
  
  INT = gl.INT,
  UNSIGNED_INT = gl.UNSIGNED_INT,

  FLOAT = gl.FLOAT,
  DOUBLE = gl.DOUBLE,
}

// The attribute layout for VAOs
// https://docs.gl/gl3/glVertexAttribPointer
Attribute_Layout :: struct {
  buffer: Buffer_Handle,

  component_size: int,  // vao size
  component_type: Attribute_Layout_Component_Type, // vao type
  stride: int,          // vao stride
  offset: int,          // vao pointer

  count: int,
}

Render_Object :: struct {
  vao: u32,
  count: int, // Number of indices
  indices: Buffer_Handle,
}

// Each Mesh is NOT a draw call.
// Meshes have RenderObjects and each RenderObject IS a draw call.
Mesh :: struct {
  render_objects: [dynamic]Render_Object,
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
  // NOTE: The indices map to buffer view indices, not buffers!
  gl_buffers: [dynamic]GL_Buffer,
  gl_attribute_layouts: [dynamic]Attribute_Layout,
}

find_cgltf_buffer_view_index :: proc(buffer_ptr: ^cgltf.buffer_view, cgltf_data: cgltf.data) -> Buffer_Handle {
  for cgltf_node_index in 0..<cgltf_data.buffer_views_count {
    if buffer_ptr == &cgltf_data.buffer_views[cgltf_node_index] {
      return Buffer_Handle(cgltf_node_index)
    }
  }
  return Invalid_Buffer_Handle
}
find_cgltf_node_index :: proc(node_ptr: ^cgltf.node, cgltf_data: cgltf.data) -> Node_Handle {
  for cgltf_node_index in 0..<cgltf_data.nodes_count {
    if node_ptr == &cgltf_data.nodes[cgltf_node_index] {
      return Node_Handle(cgltf_node_index)
    }
  }
  return Invalid_Node_Handle
}
find_cgltf_mesh_index :: proc(mesh_ptr: ^cgltf.mesh, cgltf_data: cgltf.data) -> Mesh_Handle {
  for cgltf_mesh_index in 0..<cgltf_data.meshes_count {
    if mesh_ptr == &cgltf_data.meshes[cgltf_mesh_index] {
      return Mesh_Handle(cgltf_mesh_index)
    }
  }
  return Invalid_Mesh_Handle
}
find_cgltf_scene_index :: proc(scene_ptr: ^cgltf.scene, cgltf_data: cgltf.data) -> Scene_Handle {
  for cgltf_scene_index in 0..<cgltf_data.scenes_count {
    if scene_ptr == &cgltf_data.scenes[cgltf_scene_index] {
      return Scene_Handle(cgltf_scene_index)
    }
  }
  return Invalid_Scene_Handle
}

find_cgltf_accessor_index :: proc(accessor_ptr: ^cgltf.accessor, cgltf_data: cgltf.data) -> Attribute_Layout_Handle {
  for cgltf_accessor_index in 0..<cgltf_data.accessors_count {
    if accessor_ptr == &cgltf_data.accessors[cgltf_accessor_index] {
      return Attribute_Layout_Handle(cgltf_accessor_index)
    }
  }
  return Invalid_Attribute_Layout_Handle
}

load :: proc(path: string) -> (gltf_data: GLTF_Data) {

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

  load_buffers :: proc(gltf_data: ^GLTF_Data, cgltf_data: cgltf.data) {
    dummy_vao: u32
    gl.GenVertexArrays(1, &dummy_vao)
    gl.BindVertexArray(dummy_vao)

    for buffer_view_index in 0..<cgltf_data.buffer_views_count {
      buffer_view := cgltf_data.buffer_views[buffer_view_index]
      buffer: GL_Buffer
      defer append_elem(&gltf_data.gl_buffers, buffer)

      switch buffer_view.type {
        case .vertices: buffer.type = .ARRAY_BUFFER
        case .indices: buffer.type = .ELEMENT_ARRAY_BUFFER
        case .invalid: {
          fmt.println("FIXME: Handle invalid buffer views.")
          buffer.type = .ARRAY_BUFFER
        }
      }

      gl.GenBuffers(1, &buffer.buffer)
      gl.BindBuffer(u32(buffer.type), buffer.buffer)
      
      offset := buffer_view.offset
      length := buffer_view.size

      ptr := slice.bytes_from_ptr(buffer_view.buffer.data, int(buffer_view.buffer.size))

      buffer_view_buffer_tmp := ptr[offset: offset + length]
      gl.BufferData(u32(buffer.type), int(length), &buffer_view_buffer_tmp[0], gl.STATIC_DRAW)
    }

    gl.BindVertexArray(0)
    gl.DeleteVertexArrays(1, &dummy_vao)
  }

  load_attribute_layouts :: proc(gltf_data: ^GLTF_Data, cgltf_data: cgltf.data) {
    for accessor_index in 0..<cgltf_data.accessors_count  {
      accessor := cgltf_data.accessors[accessor_index]

      attribute_layout: Attribute_Layout
      defer append_elem(&gltf_data.gl_attribute_layouts, attribute_layout)

      offset := accessor.offset
      component_type := accessor.component_type
      // Scalars or Vectors
      element_type := accessor.type
      count := accessor.count

      attribute_layout.buffer = find_cgltf_buffer_view_index(accessor.buffer_view, cgltf_data)
      attribute_layout.offset = int(offset)

      attribute_layout.component_type = Attribute_Layout_Component_Type(component_type)
      switch component_type {
        case .r_8:   attribute_layout.component_type = .BYTE
        case .r_8u:  attribute_layout.component_type = .UNSIGNED_BYTE
        case .r_16:  attribute_layout.component_type = .SHORT
        case .r_16u: attribute_layout.component_type = .UNSIGNED_SHORT
        case .r_32u: attribute_layout.component_type = .UNSIGNED_INT
        case .r_32f: attribute_layout.component_type = .FLOAT
        case .invalid: {
          fmt.eprintln("FIXME: handle invalid component types")
        }
      }
      
      switch element_type {
        case .scalar: attribute_layout.component_size = 1
        case .vec2: attribute_layout.component_size = 2
        case .vec3: attribute_layout.component_size = 3
        case .vec4: attribute_layout.component_size = 4
        case .mat2: attribute_layout.component_size = 4
        case .invalid, .mat3, .mat4: {
          fmt.eprintln("FIXME: Handle more component types")
        }
      }

      component_type_byte_size: int 
      switch attribute_layout.component_type {
        case .BYTE, .UNSIGNED_BYTE: component_type_byte_size = size_of(byte)
        case .UNSIGNED_INT, .INT: component_type_byte_size = size_of(u32)
        case .DOUBLE: component_type_byte_size = size_of(f64)
        case .FLOAT: component_type_byte_size = size_of(f32)
        case .SHORT, .UNSIGNED_SHORT: component_type_byte_size = size_of(u16)
      }

     // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#buffers-and-buffer-views-overview
     /*
     When a buffer view is used for vertex attribute data, it MAY have a byteStride property.
      This property defines the stride in bytes between each vertex.
     */
      attribute_layout.stride = int(accessor.stride)
        // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#data-alignment
        // When byteStride of the referenced bufferView is not defined, it means that accessor elements are tightly packed.
        //attribute_layout.stride = 0


      attribute_layout.count = int(count)
    }
  }

  load_meshes :: proc(gltf_data: ^GLTF_Data, cgltf_data: cgltf.data) {
    
    for mesh_index in 0..<cgltf_data.meshes_count {
      gltf_mesh := cgltf_data.meshes[mesh_index]
      mesh: Mesh
      defer append_elem(&gltf_data.meshes, mesh)
      
      for primitive_index in 0..<gltf_mesh.primitives_count {
        gltf_primitive := gltf_mesh.primitives[primitive_index]
        render_object: Render_Object
        defer append_elem(&mesh.render_objects, render_object)
  /*       if cgltf_primtive.indices == nil {
          fmt.eprintln("Handle non indexed geometry")
          continue
        } */
        gl.GenVertexArrays(1, &render_object.vao)
        gl.BindVertexArray(render_object.vao)

        for gltf_attribute_index in 0..<gltf_primitive.attributes_count {
          gltf_attribute := gltf_primitive.attributes[gltf_attribute_index]
          slot: int
           switch gltf_attribute.name {
            case "POSITION": slot = 0
            case: {
              fmt.println("FIXME: Add more attribute slots", gltf_attribute.name)
              continue
            }
          }
          //attribute_layout := gltf_data.gl_attribute_layouts[val.(json.Integer)]
          attribute_layout := gltf_data.gl_attribute_layouts[find_cgltf_accessor_index(gltf_attribute.data, cgltf_data)]
          render_object.count = attribute_layout.count
          
          buffer := gltf_data.gl_buffers[attribute_layout.buffer]
          gl.BindBuffer(gl.ARRAY_BUFFER, buffer.buffer)
          
          gl.EnableVertexAttribArray(u32(slot))
          gl.VertexAttribPointer(u32(slot), i32(attribute_layout.component_size), u32(attribute_layout.component_type),
          false, i32(attribute_layout.stride), uintptr(attribute_layout.offset))
        }

        indices_index := find_cgltf_accessor_index(gltf_primitive.indices, cgltf_data)
        indices_accessor := gltf_data.gl_attribute_layouts[indices_index]
        indices_buffer := gltf_data.gl_buffers[indices_accessor.buffer]
        gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, indices_buffer.buffer)
        gl.VertexArrayElementBuffer(render_object.vao, indices_buffer.buffer)

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

  load_buffers(&gltf_data, data^)
   // We need the accessors for when we define the VAO's layout.
  load_attribute_layouts(&gltf_data, data^)
  load_meshes(&gltf_data, data^)
  load_nodes(&gltf_data, data^)
  load_scenes(&gltf_data, data^)
  //gltf_data.default_scene = Scene_Handle(root["scene"].(json.Integer))
  return
}

// Model Loading END