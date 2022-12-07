cmake_minimum_required(VERSION 3.15)
project(stb_image)

add_library(${PROJECT_NAME} stb_image.cpp stb_image.h)
target_include_directories(${PROJECT_NAME} INTERFACE ./)