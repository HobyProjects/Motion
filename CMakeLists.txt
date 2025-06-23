cmake_minimum_required(VERSION 3.24)
project(Motion VERSION 1.0.0 LANGUAGES C CXX)

set(MOTION_CORE_LIBRARY MotionCore)
set(MOTION_APP_EXECUTABLE MotionEngine)

##############################################################################
# CMake Configuration
##############################################################################
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(PlatformConfiguration)      # This includes the function to configure platform-specific settings
include(CompilerConfiguration)      # This includes the function to set compiler configurations


##############################################################################
# Compiler Configuration
##############################################################################
SetCompilerConfigurations()

###############################################################################
# Dependencies
###############################################################################
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(soil2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(Assimp CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

###############################################################################
# Platform Specific Configuration
###############################################################################
ConfigurePlatform() 


###############################################################################
# Motion Engine - Core Library
###############################################################################
set(MOTION_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Src/Core)

set(
    MOTION_CORE_INCLUDE_DIR
        ${MOTION_CORE_DIR}/Base
        ${MOTION_CORE_DIR}/Event
        ${MOTION_CORE_DIR}/Renderer
        ${MOTION_CORE_DIR}/Screen
        ${MOTION_CORE_DIR}/UI
        ${MOTION_CORE_DIR}/ECS
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL
)

set(
    MOTION_CORE_HEADERS
        ${MOTION_CORE_DIR}/Base/Base.hpp
        ${MOTION_CORE_DIR}/Base/Asserts.hpp
        ${MOTION_CORE_DIR}/Base/Log.hpp
        ${MOTION_CORE_DIR}/Base/KeyCodes.hpp
        ${MOTION_CORE_DIR}/Base/Timer.hpp
        ${MOTION_CORE_DIR}/Base/UUID.hpp
        ${MOTION_CORE_DIR}/Base/Asset.hpp
        ${MOTION_CORE_DIR}/Base/Layer.hpp
        ${MOTION_CORE_DIR}/Base/LayersManager.hpp
        ${MOTION_CORE_DIR}/Base/AssetManager.hpp
        ${MOTION_CORE_DIR}/Base/MaterialManager.hpp
        ${MOTION_CORE_DIR}/Base/MaterialSerializer.hpp

        ${MOTION_CORE_DIR}/Event/Event.hpp
        ${MOTION_CORE_DIR}/Event/EventWindow.hpp
        ${MOTION_CORE_DIR}/Event/EventKeyboard.hpp
        ${MOTION_CORE_DIR}/Event/EventMouse.hpp
        ${MOTION_CORE_DIR}/Event/InputsHandler.hpp
        ${MOTION_CORE_DIR}/Event/GLFW_InputsHandler.hpp

        ${MOTION_CORE_DIR}/Renderer/Buffers.hpp
        ${MOTION_CORE_DIR}/Renderer/Shaders.hpp
        ${MOTION_CORE_DIR}/Renderer/Renderer.hpp
        ${MOTION_CORE_DIR}/Renderer/DrawCommand.hpp
        ${MOTION_CORE_DIR}/Renderer/Texture.hpp
        ${MOTION_CORE_DIR}/Renderer/Arrays.hpp
        ${MOTION_CORE_DIR}/Renderer/Camera.hpp
        ${MOTION_CORE_DIR}/Renderer/Camera3D.hpp
        ${MOTION_CORE_DIR}/Renderer/Model.hpp
        ${MOTION_CORE_DIR}/Renderer/Mesh.hpp
        ${MOTION_CORE_DIR}/Renderer/Material.hpp
        ${MOTION_CORE_DIR}/Renderer/Importer.hpp


        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Buffers.hpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Debug.hpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Shaders.hpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Renderer.hpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Texture.hpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Arrays.hpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GLFW_GL_Context.hpp

        ${MOTION_CORE_DIR}/Screen/Window.hpp
        ${MOTION_CORE_DIR}/Screen/GLFW_Window.hpp

        ${MOTION_CORE_DIR}/UI/UI.hpp

        ${MOTION_CORE_DIR}/ECS/Entity.hpp
        ${MOTION_CORE_DIR}/ECS/Components.hpp

)

set(
    MOTION_CORE_SOURCES
        ${MOTION_CORE_DIR}/Base/Log.cpp
        ${MOTION_CORE_DIR}/Base/AssetManager.cpp
        ${MOTION_CORE_DIR}/Base/MaterialManager.cpp
        ${MOTION_CORE_DIR}/Base/MaterialSerializer.cpp
        ${MOTION_CORE_DIR}/Base/LayersManager.cpp

        ${MOTION_CORE_DIR}/Renderer/Buffers.cpp
        ${MOTION_CORE_DIR}/Renderer/Shaders.cpp
        ${MOTION_CORE_DIR}/Renderer/Renderer.cpp
        ${MOTION_CORE_DIR}/Renderer/Arrays.cpp
        ${MOTION_CORE_DIR}/Renderer/Camera3D.cpp
        ${MOTION_CORE_DIR}/Renderer/Mesh.cpp
        ${MOTION_CORE_DIR}/Renderer/Material.cpp
        ${MOTION_CORE_DIR}/Renderer/Importer.cpp
        ${MOTION_CORE_DIR}/Renderer/Model.cpp

        ${MOTION_CORE_DIR}/Event/InputsHandler.cpp
        ${MOTION_CORE_DIR}/Event/GLFW_InputsHandler.cpp

        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Buffers.cpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Debug.cpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Shaders.cpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Renderer.cpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Texture.cpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GL_Arrays.cpp
        ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL/GLFW_GL_Context.cpp

        ${MOTION_CORE_DIR}/Screen/Window.cpp
        ${MOTION_CORE_DIR}/Screen/GLFW_Window.cpp

        ${MOTION_CORE_DIR}/UI/UI.cpp

        ${MOTION_CORE_DIR}/ECS/Entity.cpp
)

add_library(${MOTION_CORE_LIBRARY} STATIC ${MOTION_CORE_HEADERS} ${MOTION_CORE_SOURCES})
target_precompile_headers(${MOTION_CORE_LIBRARY} PUBLIC ${MOTION_CORE_DIR}/Base/CorePCH.hpp)
target_include_directories(${MOTION_CORE_LIBRARY} PUBLIC ${MOTION_CORE_INCLUDE_DIR})
target_link_libraries(
    ${MOTION_CORE_LIBRARY} 
        PUBLIC 
            glfw 
            spdlog::spdlog 
            glad::glad 
            glm::glm-header-only
            soil2::soil2
            imgui::imgui
            assimp::assimp
            yaml-cpp::yaml-cpp
            EnTT::EnTT
)

###############################################################################
# Motion Engine - Application
###############################################################################
set(MOTION_APP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Src/App)

set(
    MTION_APP_INCLUDE_DIR
        ${MOTION_APP_DIR}/EntryPoint
        ${MOTION_APP_DIR}/Layers
        ${MOTION_APP_DIR}/Camera
)

set(
    MOTION_APP_HEADERS
        ${MOTION_APP_DIR}/EntryPoint/Application.hpp
        ${MOTION_APP_DIR}/Layers/ImGuiLayer.hpp
        ${MOTION_APP_DIR}/Camera/MainCamera.hpp
)

set(
    MOTION_APP_SOURCES
        ${MOTION_APP_DIR}/EntryPoint/MotionEngine.cpp
        ${MOTION_APP_DIR}/EntryPoint/Application.cpp
        ${MOTION_APP_DIR}/Layers/ImGuiLayer.cpp
        ${MOTION_APP_DIR}/Camera/MainCamera.cpp
)

add_executable(${MOTION_APP_EXECUTABLE} ${MOTION_APP_HEADERS} ${MOTION_APP_SOURCES})
target_include_directories(${MOTION_APP_EXECUTABLE} PUBLIC ${MTION_APP_INCLUDE_DIR} ${MOTION_CORE_INCLUDE_DIR})
target_link_libraries(${MOTION_APP_EXECUTABLE} PUBLIC ${MOTION_CORE_LIBRARY})