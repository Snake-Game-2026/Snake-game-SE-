# Snake Game - builds every user-story file in features/ (US-01 .. US-19). Portable build (NFR-6).
#   cmake -S . -B build          (Windows: add -DSFML_DIR=C:/SFML/lib/cmake/SFML if needed)
#   cmake --build build
#   ctest --test-dir build       (runs every self-test with --test)
cmake_minimum_required(VERSION 3.16)
project(SnakeGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)
enable_testing()

file(GLOB STORY_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/features/*.cpp)
foreach(src ${STORY_SOURCES})
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE SFML::Graphics SFML::Window SFML::System)
    if(MSVC)
        target_compile_options(${name} PRIVATE /W4 /utf-8)
    else()
        target_compile_options(${name} PRIVATE -Wall -Wextra)
    endif()
    add_test(NAME ${name} COMMAND ${name} --test)
endforeach()
