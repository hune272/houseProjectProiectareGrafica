cmake_minimum_required(VERSION 3.20)
project(lab2 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lab2
        main.cpp
        src/ObjModel.cpp
        src/ObjModel.h
        DebugRenderer.cpp
        DebugRenderer.h
        external/tinyobj/tiny_obj_loader.cc
        external/stb/stb_image.h
)

target_compile_definitions(lab2 PRIVATE
        GLEW_STATIC
        GLEW_NO_GLU
)

target_include_directories(lab2 PRIVATE
        "C:/Users/alber/Desktop/OpenGL_dev_libs/include"
        "C:/Users/alber/Desktop/OpenGL_dev_libs/lib"
        "${CMAKE_SOURCE_DIR}/external/tinyobj"
        "${CMAKE_SOURCE_DIR}/external/stb"
        "${CMAKE_SOURCE_DIR}/src"

)

target_link_directories(lab2 PRIVATE
        "C:/Users/alber/Desktop/OpenGL_dev_libs/lib"
)

target_sources(lab2 PRIVATE
        external/tinyobj/tiny_obj_loader.cc
)

target_link_libraries(lab2 PRIVATE
        glfw3
        glew32s
        opengl32
)

set_target_properties(lab2 PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
