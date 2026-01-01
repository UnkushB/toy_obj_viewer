cmake_minimum_required(VERSION 3.10.0)
project(3DObjViewer VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glfw)

find_package( OpenGL REQUIRED )

add_executable(3DObjViewer main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glad/src/glad.c)
target_include_directories(3DObjViewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glad/include)
target_include_directories(3DObjViewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dependencies)
target_link_libraries(3DObjViewer glfw)
target_link_libraries(3DObjViewer OpenGL::GL)

set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")