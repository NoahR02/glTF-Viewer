#pragma once

#include <glad/glad.h>
#include <stb_image.h>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <functional>
#include "glm/gtc/type_ptr.hpp"
#include <filesystem>
#include <cstdint>

// Basic structures to keep gl related data together.
// The intention is not to create a OpenGL wrapper.

enum struct Buffer_Type {
  Invalid = -1,
  Array_Buffer = GL_ARRAY_BUFFER,
  Element_Array_Buffer = GL_ELEMENT_ARRAY_BUFFER,
};

struct Buffer {
  uint32_t renderer_id{};
  int target{};
};

enum struct Primitive_Mode {
  Triangles      = GL_TRIANGLES,
  Triangle_Strip = GL_TRIANGLE_STRIP,
  Triangle_Fan   = GL_TRIANGLE_FAN,

  Points = GL_POINTS,
  Lines  = GL_LINES,

  Line_Loop  = GL_LINE_LOOP,
  Line_Strip = GL_LINE_STRIP,
};

enum struct Vector_Type {
  Scalar = 1,
  Vec2 = 3,
  Vec3 = 3,
  Vec4 = 4,
};

enum struct Component_Type {
  Byte = GL_BYTE, Unsigned_Byte = GL_UNSIGNED_BYTE,
  Unsigned_Short = GL_UNSIGNED_SHORT, Short = GL_SHORT,

  Unsigned_Int = GL_UNSIGNED_INT, Int = GL_INT,

  Float = GL_FLOAT,
  Double = GL_DOUBLE,
};

/* // The attribute layout for VAOs
// https://docs.gl/gl3/glVertexAttribPointer
Attribute_Layout :: struct {
  component_size: int,  // vao size
  vector_type:    Vector_Type,
  component_type: Component_Type, // vao type
  stride: int,          // vao stride
  offset: int,          // vao pointer
} */

struct Vertex_Array {
  uint32_t renderer_id{};
  Primitive_Mode primitive_mode{};
  Component_Type indices_component_type{};
  // How many Indicies? len(indices)
  int count;

  bool has_indices{};
  int offset{};
};
