cmake_minimum_required(VERSION 3.15)
project(Motion VERSION 1.0.0 LANGUAGES C CXX)

##############################################################################
# Compiler Configuration
##############################################################################
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

###############################################################################
# Dependencies
###############################################################################
find_package(glfw3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(glad REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)
find_package(soil2 REQUIRED)
find_package(imgui REQUIRED)
find_package(EnTT REQUIRED)

###############################################################################
# Platform Specific Configuration
###############################################################################
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(MOTION_PLATFORM_WINDOWS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(MOTION_PLATFORM_LINUX)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(MOTION_DEBUG)
else()
    add_compile_definitions(MOTION_RELEASE)
endif()

set(MOTION_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Src/Core)
set(MOTION_APP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Src/App)

###############################################################################
# Motion Engine - Core Library
###############################################################################
set(
    MOTION_CORE_INCLUDE_DIR
        ${MOTION_CORE_DIR}/Base
        ${MOTION_CORE_DIR}/Event
        ${MOTION_CORE_DIR}/Renderer
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL
)

set(
    MOTION_CORE_HEADERS
        ${MOTION_CORE_DIR}/Base/Base.hpp
        ${MOTION_CORE_DIR}/Base/Asserts.hpp
        ${MOTION_CORE_DIR}/Base/Log.hpp
        ${MOTION_CORE_DIR}/Base/KeyCodes.hpp

        ${MOTION_CORE_DIR}/Event/Event.hpp
        ${MOTION_CORE_DIR}/Event/EventWindow.hpp
        ${MOTION_CORE_DIR}/Event/EventKeyboard.hpp
        ${MOTION_CORE_DIR}/Event/EventMouse.hpp

        ${MOTION_CORE_DIR}/Renderer/Buffers.hpp
        ${MOTION_CORE_DIR}/Renderer/Shaders.hpp
        ${MOTION_CORE_DIR}/Renderer/Renderer.hpp
        ${MOTION_CORE_DIR}/Renderer/Texture.hpp
        ${MOTION_CORE_DIR}/Renderer/Arrays.hpp


        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Buffers.hpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Debug.hpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Shaders.hpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Renderer.hpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Texture.hpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Arrays.hpp

)

set(
    MOTION_CORE_SOURCES
        ${MOTION_CORE_DIR}/Base/Log.cpp

        ${MOTION_CORE_DIR}/Renderer/Buffers.cpp
        ${MOTION_CORE_DIR}/Renderer/Shaders.cpp
        ${MOTION_CORE_DIR}/Renderer/Renderer.cpp
        ${MOTION_CORE_DIR}/Renderer/Texture.cpp
        ${MOTION_CORE_DIR}/Renderer/Arrays.cpp

        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Buffers.cpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Debug.cpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Shaders.cpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Renderer.cpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Texture.cpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Arrays.cpp

)

add_library(MotionCore STATIC ${MOTION_CORE_HEADERS} ${MOTION_CORE_SOURCES})
target_precompile_headers(MotionCore PUBLIC ${MOTION_CORE_DIR}/Base/CorePCH.hpp)
target_include_directories(MotionCore PUBLIC ${MOTION_CORE_INCLUDE_DIR})
target_link_libraries(
    MotionCore 
        PUBLIC 
            glfw 
            spdlog::spdlog 
            glad::glad 
            glm::glm-header-only
            assimp::assimp
            soil2::soil2
            imgui::imgui
)