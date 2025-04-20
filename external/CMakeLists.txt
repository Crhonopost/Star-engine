cmake_minimum_required(VERSION 3.15)
include(ExternalProject)


if(MSVC AND NOT "${MSVC_VERSION}" LESS 1400)
	add_definitions( "/MP" )
endif()


add_definitions(
	-DTW_STATIC
	-DTW_NO_LIB_PRAGMA
	-DTW_NO_DIRECT3D
	-DGLEW_STATIC
	-D_CRT_SECURE_NO_WARNINGS
)

### GLFW ###

add_subdirectory (glfw-3.1.2)

include_directories(
	glfw-3.1.2/include/GLFW/
	glew-1.13.0/include/
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set(OPENGL_LIBRARY
	${OPENGL_LIBRARY}
	-lGL -lGLU -lXrandr -lXext -lX11 -lrt
	${CMAKE_DL_LIBS}
	${GLFW_LIBRARIES}
)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(OPENGL_LIBRARY
	${OPENGL_LIBRARY}
	${CMAKE_DL_LIBS}
	${GLFW_LIBRARIES}
)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

### GLEW ###

set(GLEW_SOURCE
	glew-1.13.0/src/glew.c
)

set(GLEW_HEADERS
)


add_library( GLEW_1130 STATIC
	${GLEW_SOURCE}
	${GLEW_INCLUDE}
)

target_link_libraries(GLEW_1130
	${OPENGL_LIBRARY}
	${EXTRA_LIBS}
)





set(ASSIMP_BUILD_DIR ${CMAKE_BINARY_DIR}/external/assimp-build)
set(ASSIMP_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assimp)

option(ASSIMP_BUILD_TESTS "Build tests" OFF)
option(ASSIMP_BUILD_SAMPLES "Build samples" OFF)
option(ASSIMP_BUILD_ASSIMP_TOOLS "Build tools" OFF)



include(FetchContent)
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.3.1
    GIT_SHALLOW TRUE
)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
FetchContent_MakeAvailable(assimp)


if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(OPENGL_LIBRARY
        ${OPENGL_LIBRARY}
        -lGL -lGLU -lXrandr -lXext -lX11 -lrt
        ${CMAKE_DL_LIBS}
        ${GLFW_LIBRARIES}
    )
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(OPENGL_LIBRARY
        ${OPENGL_LIBRARY}
        ${CMAKE_DL_LIBS}
        ${GLFW_LIBRARIES}
    )
endif()

target_link_libraries(GLEW_1130 ${OPENGL_LIBRARY} ${EXTRA_LIBS})