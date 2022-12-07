cmake_minimum_required(VERSION 3.15)
# Note: Docking Branch!
project(imgui)

add_library(${PROJECT_NAME} STATIC
        imgui.h
        imgui.cpp

        imstb_textedit.h
        imstb_truetype.h
        imgui_widgets.cpp
        imgui_draw.cpp

        imgui_impl_opengl3.h
        imgui_impl_opengl3.cpp

        imgui_impl_glfw.h
        imgui_impl_glfw.cpp

        imgui_tables.cpp
)

# If you cannot link with opengl using imgui, you need this.
target_compile_definitions(${PROJECT_NAME} PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)

target_include_directories(${PROJECT_NAME} INTERFACE ../)
target_link_libraries(${PROJECT_NAME} glad)
target_link_libraries(${PROJECT_NAME} glfw)