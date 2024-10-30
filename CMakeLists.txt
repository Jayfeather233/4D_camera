cmake_minimum_required(VERSION 3.11)

project(4DCam)

add_definitions(-DGLM_ENABLE_CXX_17 -DGLM_ENABLE_SIMD_AVX2)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(fmt REQUIRED)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

include_directories(${fmt_INCLUDE_DIRS})
include_directories("./src")
include_directories("./src/opengl")
include_directories("./src/objects")
include_directories(${GLFW_INCLUDE_DIRS})
include_directories(${OPENGL_INCLUDE_DIRS})
include_directories(${GLEW_INCLUDE_DIRS})

aux_source_directory(./src MAIN_SOURCES)
aux_source_directory(./src/opengl OPENGL_SOURCES)
aux_source_directory(./src/objects OBJ_SOURCES)

add_executable(${PROJECT_NAME}  ${OPENGL_SOURCES} ${MAIN_SOURCES} ${OBJ_SOURCES})

target_link_libraries(${PROJECT_NAME} PUBLIC glfw)
target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENGL_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${GLEW_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt)
target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen)