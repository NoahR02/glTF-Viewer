cmake_minimum_required(VERSION 3.15)
project(game)

add_executable(${PROJECT_NAME} src/main.cpp src/renderer/renderer.h src/editor_camera.h src/input.h src/renderer/gltf.h src/renderer/gl.h src/window/window.h src/window/active_window.h src/window/active_window.cpp)

include(FetchContent)

FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
)
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
)
FetchContent_Declare(
        tinygltf
        GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
        GIT_TAG origin/release
)
FetchContent_MakeAvailable(glad glfw glm tinygltf)
add_subdirectory(third-party)
target_link_libraries(${PROJECT_NAME} glad glfw glm tinygltf stb_image imgui)
# Copy Assets directory to the build folder.
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets)