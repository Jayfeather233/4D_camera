cmake_minimum_required(VERSION 3.16)

project(4DCam LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# This project is unusably slow without optimization, so never default to a
# debug/unoptimized build.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

option(4DCAM_NATIVE_ARCH "Optimize for the build machine (-march=native)" ON)
option(4DCAM_BUILD_TOOLS "Build the headless preview tool" OFF)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(fmt REQUIRED)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

if(TARGET GLEW::GLEW)
    set(4DCAM_GLEW GLEW::GLEW)
elseif(TARGET GLEW::glew)
    set(4DCAM_GLEW GLEW::glew)
elseif(TARGET GLEW::glew_s)
    set(4DCAM_GLEW GLEW::glew_s)
else()
    set(4DCAM_GLEW ${GLEW_LIBRARIES})
    include_directories(${GLEW_INCLUDE_DIRS})
endif()

if(TARGET glfw)
    set(4DCAM_GLFW glfw)
elseif(TARGET glfw3)
    set(4DCAM_GLFW glfw3)
else()
    set(4DCAM_GLFW ${GLFW_LIBRARIES})
endif()

# ------------------------------------------------------------------- core
# Everything except the window/input loop, so that tools can reuse it.
add_library(4dcam_core
    src/objects/mesh_data.cpp
    src/opengl/mesh.cpp
    src/opengl/shader.cpp
    src/opengl/camera3D.cpp
    src/opengl/camera4D.cpp
    src/scene.cpp
    src/overlay.cpp
)
target_include_directories(4dcam_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(4dcam_core PUBLIC GLM_FORCE_CXX17)
# Shaders are read from the source tree; override at runtime with FOURDCAM_SHADER_DIR.
target_compile_definitions(4dcam_core PRIVATE FOURDCAM_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders")
target_link_libraries(4dcam_core PUBLIC OpenGL::GL Eigen3::Eigen fmt::fmt ${4DCAM_GLEW})

if(NOT MSVC)
    set(4DCAM_WARNINGS -Wall -Wextra)
    target_compile_options(4dcam_core PRIVATE ${4DCAM_WARNINGS})
    if(4DCAM_NATIVE_ARCH)
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag("-march=native" FOURDCAM_HAS_MARCH_NATIVE)
        if(FOURDCAM_HAS_MARCH_NATIVE)
            target_compile_options(4dcam_core PUBLIC -march=native)
        endif()
    endif()
endif()

# ------------------------------------------------------------- application
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE 4dcam_core ${4DCAM_GLFW})
if(NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE ${4DCAM_WARNINGS})
endif()

# ------------------------------------------------------------------- tools
# 4dcam_preview needs the EGL surfaceless platform (Linux/Mesa).
if(4DCAM_BUILD_TOOLS)
    if(UNIX AND NOT APPLE)
        add_executable(4dcam_preview tools/preview.cpp)
        target_link_libraries(4dcam_preview PRIVATE 4dcam_core EGL)
        if(NOT MSVC)
            target_compile_options(4dcam_preview PRIVATE ${4DCAM_WARNINGS})
        endif()
    else()
        message(STATUS "4DCAM_BUILD_TOOLS: 4dcam_preview is only available on Linux")
    endif()
endif()
