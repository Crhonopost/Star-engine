# external/imgui/CMakeLists.txt
project(imgui)

file(GLOB IMGUI_SOURCES *.cpp)
file(GLOB IMGUI_BACKENDS *.cpp)

add_library(imgui STATIC
    ${IMGUI_SOURCES}
    ${IMGUI_BACKENDS}
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(imgui PUBLIC
    IMGUI_IMPL_OPENGL_LOADER_GLEW
)