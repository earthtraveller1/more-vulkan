cmake_minimum_required(VERSION 3.13)

project("more-vulkan" LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)

FetchContent_MakeAvailable(glfw glm)

find_package(Vulkan)

aux_source_directory(src SOURCES)
add_executable(${PROJECT_NAME} ${SOURCES})

file(GLOB SHADERS shaders/*.vert shaders/*.frag)
foreach(SHADER ${SHADERS})
    add_custom_command(
        OUTPUT ${SHADER}.spv
        COMMAND glslc -o ${SHADER}.spv ${SHADER}
        DEPENDS ${SHADER}
    )
    target_sources(${PROJECT_NAME} PRIVATE ${SHADER}.spv)
endforeach()

target_link_libraries(${PROJECT_NAME} PRIVATE glfw Vulkan::Vulkan glm)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/precompiled.hpp)

if (NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
