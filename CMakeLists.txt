cmake_minimum_required(VERSION 3.13)

project("more-vulkan" LANGUAGES CXX VERSION 0.1.0)

aux_source_directory(src SOURCES)
add_executable(${PROJECT_NAME} ${SOURCES})

if (NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
