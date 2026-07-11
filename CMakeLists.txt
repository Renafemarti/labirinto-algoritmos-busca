cmake_minimum_required(VERSION 3.16)
project(HelloWindow LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encontra as bibliotecas instaladas no seu Linux
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(HelloWindow
    src/labirinto.cpp
    src/Maze.cpp
    src/Renderer.cpp
    src/Solver.cpp
    external/glad/src/glad.c
)

# A SOLUÇÃO: Usando caminhos absolutos para o compilador achar os .hpp
target_include_directories(HelloWindow PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/glad/include
    ${CMAKE_SOURCE_DIR}/external/glm
)

target_link_libraries(HelloWindow PRIVATE
    glfw
    OpenGL::GL
)