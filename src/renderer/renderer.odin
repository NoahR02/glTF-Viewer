package renderer

import "gltf"
import "core:strings"
import gl "vendor:OpenGL"
import "core:fmt"
import "core:slice"
import "core:mem"
import "core:encoding/json"
import "core:encoding/base64"
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

load :: proc(path: string) -> (gltf_data: GLTF_Data) {

  json_gltf_data := gltf.load(path)

  load_buffers :: proc(gltf_data: ^GLTF_Data, json_gltf: json.Object) {
    dummy_vao: u32
    gl.GenVertexArrays(1, &dummy_vao)
    gl.BindVertexArray(dummy_vao)

    for buffer_view in json_gltf["bufferViews"].(json.Array) {
      buffer_view := buffer_view.(json.Object)
      buffer: GL_Buffer
      defer append_elem(&gltf_data.gl_buffers, buffer)


      buffer.type = GL_Buffer_Type(buffer_view["target"].(json.Integer))
      gl.GenBuffers(1, &buffer.buffer)
      gl.BindBuffer(u32(buffer.type), buffer.buffer)

      gltf_json_buffer := json_gltf["buffers"].(json.Array)[buffer_view["buffer"].(json.Integer)]
      
      offset := buffer_view["byteOffset"].(json.Integer)
      length := buffer_view["byteLength"].(json.Integer)

      ptr := gltf.decode_buffer(gltf_json_buffer)
      buffer_view_buffer_tmp := ptr[offset: offset + length]
      gl.BufferData(u32(buffer.type), int(length), &buffer_view_buffer_tmp[0], gl.STATIC_DRAW)
    }

    gl.BindVertexArray(0)
    gl.DeleteVertexArrays(1, &dummy_vao)
  }

  load_attribute_layouts :: proc(gltf_data: ^GLTF_Data, json_gltf: json.Object) {
    for accessor in json_gltf["accessors"].(json.Array)  {
      accessor := accessor.(json.Object)

      attribute_layout: Attribute_Layout
      defer append_elem(&gltf_data.gl_attribute_layouts, attribute_layout)

      offset := accessor["byteOffset"].(json.Integer)
      component_type := accessor["componentType"].(json.Integer)
      // Scalars or Vectors
      element_type := accessor["type"].(json.String)
      count := accessor["count"].(json.Integer)

      attribute_layout.buffer = Buffer_Handle(accessor["bufferView"].(json.Integer))
      attribute_layout.offset = int(offset)

      attribute_layout.component_type = Attribute_Layout_Component_Type(component_type)
      switch element_type {
        case "SCALAR": attribute_layout.component_size = 1
        case "VEC2": attribute_layout.component_size = 2
        case "VEC3": attribute_layout.component_size = 3
        case "VEC4": attribute_layout.component_size = 4
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
      byte_stride, has_byte_stride := accessor["byteStride"]
      if has_byte_stride {
        attribute_layout.stride = int(byte_stride.(json.Integer))
      } else {
        // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#data-alignment
        // When byteStride of the referenced bufferView is not defined, it means that accessor elements are tightly packed.
        attribute_layout.stride = 0
      }

      attribute_layout.count = int(count)
    }
  }

  load_meshes :: proc(gltf_data: ^GLTF_Data, json_gltf: json.Object) {
    
    for json_mesh in json_gltf["meshes"].(json.Array) {
      json_mesh := json_mesh.(json.Object)
      mesh: Mesh
      defer append_elem(&gltf_data.meshes, mesh)
      
      for json_primitive in json_mesh["primitives"].(json.Array) {
        json_primitive := json_primitive.(json.Object)
        render_object: Render_Object
        defer append_elem(&mesh.render_objects, render_object)
  /*       if cgltf_primtive.indices == nil {
          fmt.eprintln("Handle non indexed geometry")
          continue
        } */
        gl.GenVertexArrays(1, &render_object.vao)
        gl.BindVertexArray(render_object.vao)

        for json_attribute, val in json_primitive["attributes"].(json.Object) {
          slot: int
         /*  switch cgltf_attribute.name {
            case "POSITION": slot = 0
          } */
          attribute_layout := gltf_data.gl_attribute_layouts[val.(json.Integer)]
          render_object.count = attribute_layout.count
          
          buffer := gltf_data.gl_buffers[attribute_layout.buffer]
          gl.BindBuffer(gl.ARRAY_BUFFER, buffer.buffer)
          
          gl.EnableVertexAttribArray(u32(slot))
          gl.VertexAttribPointer(u32(slot), i32(attribute_layout.component_size), u32(attribute_layout.component_type),
          false, i32(attribute_layout.stride), uintptr(attribute_layout.offset))
        }

        indices_accessor := gltf_data.gl_attribute_layouts[json_primitive["indices"].(json.Integer)]
        indices_buffer := gltf_data.gl_buffers[indices_accessor.buffer]
        gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, indices_buffer.buffer)
        gl.VertexArrayElementBuffer(render_object.vao, indices_buffer.buffer)

      }
    }

  }

  load_nodes :: proc(gltf_data: ^GLTF_Data, json_gltf: json.Object) {
    for json_node in json_gltf["nodes"].(json.Array) {
      json_node := json_node.(json.Object)
      node: Node
      defer append_elem(&gltf_data.nodes, node)

      node.mesh = Mesh_Handle(json_node["mesh"].(json.Integer))

      /* for json_child_node in json_node["children"].(json.Array) {
        json_child_node := json_child_node.(json.Integer)
        append_elem(&node.children, Node_Handle(json_child_node))
      } */
      
    }
  }


  load_scenes :: proc(gltf_data: ^GLTF_Data, json_gltf: json.Object) {
    for json_scene in json_gltf["scenes"].(json.Array) {
      json_scene := json_scene.(json.Object)
      scene: Scene
      defer append_elem(&gltf_data.scenes, scene)

      for node in json_scene["nodes"].(json.Array) {
        node := node.(json.Integer)
        append_elem(&scene.nodes, Node_Handle(node))
      }
      
    }
  }

  root := json_gltf_data.(json.Object)
  load_buffers(&gltf_data, root)
   // We need the accessors for when we define the VAO's layout.
  load_attribute_layouts(&gltf_data, root)
  load_meshes(&gltf_data, root)
  load_nodes(&gltf_data, root)
  load_scenes(&gltf_data, root)
  gltf_data.default_scene = Scene_Handle(root["scene"].(json.Integer))
  //fmt.println(gltf_data)
  return
}

// Model Loading END