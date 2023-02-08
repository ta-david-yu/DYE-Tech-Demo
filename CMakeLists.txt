cmake_minimum_required(VERSION 3.24)
project(DYETechDemo)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(DYEngine/cmake/Functions/RedirectOutputTo.cmake)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    RedirectOutputTo("out/Debug")
    add_compile_definitions(DYE_DEBUG)
    add_compile_definitions(DYE_OPENGL_DEBUG)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    RedirectOutputTo("out/Release")
endif ()

add_subdirectory(DYEngine/DYEngine)

add_executable(DYETechDemo src/DYETechDemoApp.cpp)

target_include_directories(DYETechDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(DYETechDemo DYEngine)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")
target_link_libraries(DYETechDemo -static-libgcc)
