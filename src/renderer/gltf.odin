package renderer

import "../../third-party/cgltf"
import "core:strings"
import gl "vendor:OpenGL"
import "core:math/linalg/glsl"
import "core:fmt"
import "core:slice"
import "core:mem"
import "core:runtime"

// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html

Node_Handle :: distinct int
Scene_Handle :: distinct int
Mesh_Handle :: distinct int
Buffer_View_Handle :: distinct int
Accessor_Handle :: distinct int

Invalid_Node_Handle :: Node_Handle(-1)
Invalid_Scene_Handle :: Scene_Handle(-1)
Invalid_Mesh_Handle :: Mesh_Handle(-1)
Invalid_Buffer_View_Handle :: Buffer_View_Handle(-1)
Invalid_Accessor_Handle :: Accessor_Handle(-1)

// The attribute layout for VAOs
// https://docs.gl/gl3/glVertexAttribPointer
Accessor :: struct {
	buffer:         Buffer_View_Handle,
	component_size: int, // vao size
	component_type: Component_Type, // vao type
	vector_type:    Vector_Type,
	stride:         int, // vao stride
	offset:         int, // vao pointer
	count:          int,
}

Transform :: struct {
  rotation: glsl.quat,
  scale: glsl.vec3,
  translation: glsl.vec3,
}

Material :: struct {
  
}

// Each Mesh is NOT a draw call.
// Meshes have RenderObjects and each RenderObject IS a draw call.
Mesh :: struct {
	name:           string,
	render_objects: [dynamic]Vertex_Array,
  material:       Material,
}

Node :: struct {
	name:     string,
	mesh:     Mesh_Handle,
	children: [dynamic]Node_Handle,
  transform: Transform,
}

Scene :: struct {
	name:  string,
	nodes: [dynamic]Node_Handle,
}

GLTF_Data :: struct {
	default_scene:     Scene_Handle,
	// All Scenes, Nodes, and Meshes.
	// NOTE: Scenes can share nodes.
	scenes:            [dynamic]Scene,
	nodes:             [dynamic]Node,
	meshes:            [dynamic]Mesh,
	accessors:         [dynamic]Accessor,
  tmp_map: map[rawptr]Node_Handle,

	// NOTE: The indices map to glTF buffer view indices, not glTF buffers!
	// gl_buffers[0] -> cgltf_data.buffer_views[0]
	gl_buffers:        [dynamic]Buffer,
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
load_scenes :: proc(gltf_data: ^GLTF_Data, cgltf_data: cgltf.data) {
	for scene_index in 0 ..< cgltf_data.scenes_count {
		gltf_scene := cgltf_data.scenes[scene_index]
		scene: ^Scene = &gltf_data.scenes[scene_index]

		for node_index in 0 ..< gltf_scene.nodes_count {
			cgltf_node: ^cgltf.node = &cgltf_data.nodes[node_index]
			append_elem(&scene.nodes, gltf_data.tmp_map[cgltf_node])
		}

	}
}

@(private)
load_buffer :: proc(
	gltf_data: ^GLTF_Data,
	cgltf_data: cgltf.data,
	buffer_view: ^cgltf.buffer_view,
) {
	buffer_view_handle := find_cgltf_buffer_view_index(buffer_view, cgltf_data)
	if buffer_view_handle == Invalid_Buffer_View_Handle {
		fmt.eprintln("FIXME: Handle buffer views not being found")
	}

	// If we have already uploaded the buffer before just return.
	if gltf_data.gl_buffers[int(buffer_view_handle)].renderer_id != 0 {
		return
	}

	buffer: ^Buffer = &gltf_data.gl_buffers[int(buffer_view_handle)]
	switch buffer_view.type {
	case .vertices:
		buffer.type = .Array_Buffer
	case .indices:
		buffer.type = .Element_Array_Buffer
	case .invalid:
		// If no target is present, default to Element_Array_Buffer
		buffer.type = .Element_Array_Buffer
	}
	gl.GenBuffers(1, &buffer.renderer_id)
	gl.BindBuffer(u32(buffer.type), buffer.renderer_id)

	raw_buffer := slice.bytes_from_ptr(buffer_view.buffer.data, int(buffer_view.buffer.size))

	buffer_view_buffer := raw_buffer[buffer_view.offset:buffer_view.offset + buffer_view.size]
	gl.BufferData(u32(buffer.type), int(buffer_view.size), &buffer_view_buffer[0], gl.STATIC_DRAW)
}

@(private)
load_attribute_layouts :: proc(gltf_data: ^GLTF_Data, cgltf_data: cgltf.data) {

	// Get all the data we need to configure the Vertex Attributes later on.
	for accessor_index in 0 ..< cgltf_data.accessors_count {
		cgltf_accessor := cgltf_data.accessors[accessor_index]

		accessor: ^Accessor = &gltf_data.accessors[accessor_index]

		accessor.buffer = find_cgltf_buffer_view_index(cgltf_accessor.buffer_view, cgltf_data)
		accessor.offset = int(cgltf_accessor.offset)
		accessor.component_type = cgltf_component_type_to_component_type(
			cgltf_accessor.component_type,
		)
		accessor.vector_type = cgltf_vector_type_to_vector_type(cgltf_accessor.type)
		accessor.stride = int(cgltf_accessor.stride)
		accessor.count = int(cgltf_accessor.count)
	}

}

@(private)
load_mesh :: proc(gltf_data: ^GLTF_Data, cgltf_data: cgltf.data, gltf_mesh: ^cgltf.mesh) {
  mesh_index := find_cgltf_mesh_index(gltf_mesh, cgltf_data)
	mesh: ^Mesh = &gltf_data.meshes[mesh_index]
	for primitive_index in 0..< gltf_mesh.primitives_count {
		gltf_primitive := gltf_mesh.primitives[primitive_index]
		vao: Vertex_Array
		defer append_elem(&mesh.render_objects, vao)

		gl.GenVertexArrays(1, &vao.renderer_id)
		gl.BindVertexArray(vao.renderer_id)

    accessor_draw_count := 0
		for gltf_attribute_index in 0 ..< gltf_primitive.attributes_count {
			gltf_attribute := gltf_primitive.attributes[gltf_attribute_index]

			slot := -1
			switch gltf_attribute.name {
			case "POSITION":
				slot = 0
      case "NORMAL":
        slot = 1
      case "TANGENT":
        slot = 2  
      case "TEXCOORD_0":
        slot = 3  
      case "WEIGHTS_0":  
        slot = 4
      case "JOINTS_0":  
        slot = 5        
			case: {
        fmt.println("FIXME: Add more attribute slots", gltf_attribute.name)
        continue
      }
			}

			accessors :=
				gltf_data.accessors[
					find_cgltf_accessor_index(gltf_attribute.data, cgltf_data) \
				]

			load_buffer(gltf_data, cgltf_data, gltf_attribute.data.buffer_view)
			buffer := gltf_data.gl_buffers[accessors.buffer]
			gl.BindBuffer(gl.ARRAY_BUFFER, buffer.renderer_id)

			gl.EnableVertexAttribArray(u32(slot))
			gl.VertexAttribPointer(
				u32(slot),
				i32(vector_type_to_number_of_components(accessors.vector_type)),
				u32(accessors.component_type),
				gltf_attribute.data.normalized ? gl.TRUE : gl.FALSE,
				i32(gltf_attribute.data.buffer_view.stride),
				uintptr(gltf_attribute.data.offset),
			)
      accessor_draw_count = int(gltf_attribute.data.count)
		}

		if gltf_primitive.indices == nil {
			vao.has_indices = false
      // When we are not working with indexed geometry, then use the accessor count which should be the same for each attribute's accessor.
      vao.count = accessor_draw_count
		} else {
			vao.has_indices = true
			indices_accessor :=
				gltf_data.accessors[
					find_cgltf_accessor_index(gltf_primitive.indices, cgltf_data) \
				]
			load_buffer(gltf_data, cgltf_data, gltf_primitive.indices.buffer_view)
			indices_buffer := gltf_data.gl_buffers[indices_accessor.buffer]
			gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, indices_buffer.renderer_id)
			vao.indices_component_type = indices_accessor.component_type
			vao.primitive_mode = cgltf_primitive_type_to_primitive_type(gltf_primitive.type)
			vao.count = indices_accessor.count
      vao.offset = indices_accessor.offset
		}

	}

}

@(private)
load_nodes :: proc(gltf_data: ^GLTF_Data, cgltf_data: cgltf.data) {

  load_node :: proc(cgltf_node: ^cgltf.node, gltf_data: ^GLTF_Data, cgltf_data: cgltf.data) {
		
    node: ^Node
    node_handle := find_cgltf_node_index(cgltf_node, cgltf_data)
    if node_handle == Invalid_Node_Handle {
      new_handle := Node_Handle(len(gltf_data.nodes))
      append_elem(&gltf_data.nodes, Node{})
      node = &gltf_data.nodes[new_handle]
      gltf_data.tmp_map[cgltf_node] = new_handle
    } else {
      node = &gltf_data.nodes[node_handle]
      gltf_data.tmp_map[cgltf_node] = node_handle
    }

    node.mesh = find_cgltf_mesh_index(cgltf_node.mesh, cgltf_data)
    node.name = strings.clone_from_cstring(cgltf_node.name)
    if node.mesh != Invalid_Mesh_Handle do load_mesh(gltf_data, cgltf_data, cgltf_node.mesh)

    if cgltf_node.has_translation {
      node.transform.translation = cgltf_node.translation.xyz
    }

		for child_node_index in 0 ..< cgltf_node.children_count {
      cgltf_child_node: ^cgltf.node = cgltf_node.children[child_node_index] 

      load_node(cgltf_child_node, gltf_data, cgltf_data)
      new_child_node_handle := gltf_data.tmp_map[cgltf_child_node]
      append_elem(&node.children, new_child_node_handle)
		}
  }

	for node_index in 0 ..< cgltf_data.nodes_count {
    load_node(&cgltf_data.nodes[node_index], gltf_data, cgltf_data)
  }

}

@(private)
internal_gltf_load :: proc(gltf_data: ^GLTF_Data, cgltf_data: cgltf.data) {
	resize_dynamic_array(&gltf_data.scenes, int(cgltf_data.scenes_count))
	resize_dynamic_array(&gltf_data.meshes, int(cgltf_data.meshes_count))
	resize_dynamic_array(&gltf_data.nodes, int(cgltf_data.nodes_count))
	resize_dynamic_array(&gltf_data.accessors, int(cgltf_data.accessors_count))
	resize_dynamic_array(&gltf_data.gl_buffers, int(cgltf_data.buffer_views_count))

	load_scenes(gltf_data, cgltf_data)
	load_attribute_layouts(gltf_data, cgltf_data)
	load_nodes(gltf_data, cgltf_data)
	gltf_data.default_scene = find_cgltf_scene_index(cgltf_data.scene, cgltf_data)
}

gltf_draw_all_scenes :: proc(gltf_data: GLTF_Data, shader: u32) {
	
  for scene in gltf_data.scenes {
    for node_index in 0..<len(scene.nodes) {
      node := gltf_data.nodes[node_index]
      if node.mesh == -1 do continue
      for render_object in gltf_data.meshes[node.mesh].render_objects {
        gl.BindVertexArray(render_object.renderer_id)
        model := glsl.mat4Translate(node.transform.translation)
        //model := glsl.identity(glsl.mat4)
        gl.UniformMatrix4fv(gl.GetUniformLocation(shader, "u_model"), 1, gl.FALSE, &model[0][0])
        if render_object.has_indices {
          gl.DrawElements(
            u32(render_object.primitive_mode),
            i32(render_object.count),
            u32(render_object.indices_component_type),
            // The byte offset FROM the start of the buffer view.
            rawptr(uintptr(render_object.offset)),
          )
        } else {
          gl.DrawArrays(gl.TRIANGLES, 0, i32(render_object.count))
        }
      }
    }
  }

}
