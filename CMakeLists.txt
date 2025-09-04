cmake_minimum_required(VERSION 3.24)
project(Motion VERSION 1.0.0 LANGUAGES C CXX)

set(MOTION_CORE_LIBRARY MotionCore)
set(MOTION_APP_EXECUTABLE MotionEngine)

# #############################################################################
# CMake Configuration
# #############################################################################
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ##############################################################################
# Dependencies
# ##############################################################################
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(Assimp CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(stb CONFIG REQUIRED)
find_package(fastgltf CONFIG REQUIRED)
find_package(MikkTSpace CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)

# ##############################################################################
# Motion Engine - Core Library
# ##############################################################################
set(MOTION_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Src/Core)

set(
    MOTION_CORE_INCLUDE_DIR
    ${MOTION_CORE_DIR}/Base
    ${MOTION_CORE_DIR}/Event
    ${MOTION_CORE_DIR}/Renderer
    ${MOTION_CORE_DIR}/Window
    ${MOTION_CORE_DIR}/UI
    ${MOTION_CORE_DIR}/ECS
    ${MOTION_CORE_DIR}/Material
    ${MOTION_CORE_DIR}/Model
    ${MOTION_CORE_DIR}/Maths
    ${MOTION_CORE_DIR}/Assets
    ${MOTION_CORE_DIR}/Layer
    ${MOTION_CORE_DIR}/Platform
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core
)

set(
    MOTION_CORE_HEADERS
    ${MOTION_CORE_DIR}/Base/Base.hpp
    ${MOTION_CORE_DIR}/Base/Asserts.hpp
    ${MOTION_CORE_DIR}/Base/Log.hpp
    ${MOTION_CORE_DIR}/Base/KeyCodes.hpp
    ${MOTION_CORE_DIR}/Base/Timer.hpp
    ${MOTION_CORE_DIR}/Base/UUID.hpp
    ${MOTION_CORE_DIR}/Base/BackgroundWorker.hpp
    ${MOTION_CORE_DIR}/Base/MainThreadDispatcher.hpp
    ${MOTION_CORE_DIR}/Base/TaskManager.hpp

    ${MOTION_CORE_DIR}/Layer/Layer.hpp
    ${MOTION_CORE_DIR}/Layer/LayersManager.hpp

    ${MOTION_CORE_DIR}/Assets/Asset.hpp
    ${MOTION_CORE_DIR}/Assets/AssetManager.hpp

    ${MOTION_CORE_DIR}/Event/Event.hpp
    ${MOTION_CORE_DIR}/Event/EventWindow.hpp
    ${MOTION_CORE_DIR}/Event/EventKeyboard.hpp
    ${MOTION_CORE_DIR}/Event/EventMouse.hpp
    ${MOTION_CORE_DIR}/Event/EventRegistry.hpp
    ${MOTION_CORE_DIR}/Event/InputsHandler.hpp
    ${MOTION_CORE_DIR}/Event/GLFW_InputsHandler.hpp

    ${MOTION_CORE_DIR}/Renderer/Buffers.hpp
    ${MOTION_CORE_DIR}/Renderer/Shaders.hpp
    ${MOTION_CORE_DIR}/Renderer/Renderer.hpp
    ${MOTION_CORE_DIR}/Renderer/Texture.hpp
    ${MOTION_CORE_DIR}/Renderer/Arrays.hpp
    ${MOTION_CORE_DIR}/Renderer/Camera.hpp
    ${MOTION_CORE_DIR}/Renderer/Camera3D.hpp
    ${MOTION_CORE_DIR}/Renderer/Thumbnail.hpp
    ${MOTION_CORE_DIR}/Renderer/PostProcessor.hpp
    ${MOTION_CORE_DIR}/Renderer/Environment.hpp
    ${MOTION_CORE_DIR}/Renderer/RenderingStage.hpp
    ${MOTION_CORE_DIR}/Renderer/RenderCommand.hpp
    ${MOTION_CORE_DIR}/Renderer/CommandQueue.hpp

    ${MOTION_CORE_DIR}/Model/Model.hpp
    ${MOTION_CORE_DIR}/Model/Mesh.hpp
    ${MOTION_CORE_DIR}/Model/Importer.hpp

    ${MOTION_CORE_DIR}/Material/Material.hpp
    ${MOTION_CORE_DIR}/Material/MaterialComponents.hpp

    ${MOTION_CORE_DIR}/Maths/Intersect.hpp
    ${MOTION_CORE_DIR}/Maths/MeshFit.hpp

    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GL_Buffers.hpp
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GL_Debug.hpp
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GL_Shaders.hpp
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GL_Renderer.hpp
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GL_Texture.hpp
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GL_Arrays.hpp
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GL_Environment.hpp
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GLFW_GL_Context.hpp
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GL_RenderingStage.hpp

    ${MOTION_CORE_DIR}/Window/Window.hpp
    ${MOTION_CORE_DIR}/Window/GLFW_Window.hpp

    ${MOTION_CORE_DIR}/UI/UI.hpp
    ${MOTION_CORE_DIR}/UI/Controls.hpp
    ${MOTION_CORE_DIR}/UI/MaterialFonts/IconsMaterialDesign.h
    ${MOTION_CORE_DIR}/Platform/DialogBoxes.hpp

    ${MOTION_CORE_DIR}/ECS/Entity.hpp
    ${MOTION_CORE_DIR}/ECS/Components.hpp
)

set(
    MOTION_CORE_SOURCES
    ${MOTION_CORE_DIR}/Base/Log.cpp
    ${MOTION_CORE_DIR}/Assets/AssetManager.cpp
    ${MOTION_CORE_DIR}/Layer/LayersManager.cpp

    ${MOTION_CORE_DIR}/Renderer/Buffers.cpp
    ${MOTION_CORE_DIR}/Renderer/Shaders.cpp
    ${MOTION_CORE_DIR}/Renderer/Texture.cpp
    ${MOTION_CORE_DIR}/Renderer/Renderer.cpp
    ${MOTION_CORE_DIR}/Renderer/Arrays.cpp
    ${MOTION_CORE_DIR}/Renderer/Camera3D.cpp
    ${MOTION_CORE_DIR}/Renderer/Thumbnail.cpp
    ${MOTION_CORE_DIR}/Renderer/PostProcessor.cpp
    ${MOTION_CORE_DIR}/Renderer/Environment.cpp
    ${MOTION_CORE_DIR}/Renderer/RenderingStage.cpp
    ${MOTION_CORE_DIR}/Renderer/RenderCommand.cpp

    ${MOTION_CORE_DIR}/Model/Mesh.cpp
    ${MOTION_CORE_DIR}/Model/Importer.cpp
    ${MOTION_CORE_DIR}/Model/Model.cpp

    ${MOTION_CORE_DIR}/Material/Material.cpp
    ${MOTION_CORE_DIR}/Material/MaterialComponents.cpp

    ${MOTION_CORE_DIR}/Event/InputsHandler.cpp
    ${MOTION_CORE_DIR}/Event/GLFW_InputsHandler.cpp

    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GL_Buffers.cpp
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GL_Debug.cpp
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GL_Shaders.cpp
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GL_Renderer.cpp
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GL_Texture.cpp
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GL_Arrays.cpp
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GL_Environment.cpp
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GLFW_GL_Context.cpp
    ${MOTION_CORE_DIR}/Renderer/APIs/OpenGL_46_Core/GL_RenderingStage.cpp

    ${MOTION_CORE_DIR}/Window/Window.cpp
    ${MOTION_CORE_DIR}/Window/GLFW_Window.cpp

    ${MOTION_CORE_DIR}/UI/UI.cpp
    ${MOTION_CORE_DIR}/UI/Controls.cpp
    ${MOTION_CORE_DIR}/Platform/DialogBoxes.cpp
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
    imgui::imgui
    assimp::assimp
    yaml-cpp::yaml-cpp
    EnTT::EnTT
    stb::stb
    fastgltf::fastgltf
    MikkTSpace::MikkTSpace
    imguizmo::imguizmo
)

# ##############################################################################
# Motion Engine - Application
# ##############################################################################
set(MOTION_APP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Src/App)

set(
    MTION_APP_INCLUDE_DIR
    ${MOTION_APP_DIR}/EntryPoint
    ${MOTION_APP_DIR}/Layers
    ${MOTION_APP_DIR}/Camera
    ${MOTION_APP_DIR}/Scene
    ${MOTION_APP_DIR}/Scene/ScenePanels
)

set(
    MOTION_APP_HEADERS
    ${MOTION_APP_DIR}/EntryPoint/Application.hpp
    ${MOTION_APP_DIR}/Layers/ImGuiLayer.hpp
    ${MOTION_APP_DIR}/Layers/SceneEditorLayer.hpp
    ${MOTION_APP_DIR}/Scene/Scene.hpp
    ${MOTION_APP_DIR}/Scene/SceneCamera.hpp
    ${MOTION_APP_DIR}/Scene/SceneRenderer.hpp
    ${MOTION_APP_DIR}/Scene/SceneEnviroment.hpp

    ${MOTION_APP_DIR}/Scene/ScenePanels/ViewportPanel.hpp
    ${MOTION_APP_DIR}/Scene/ScenePanels/SettingsPanel.hpp
    ${MOTION_APP_DIR}/Scene/ScenePanels/EntityInspectPanel.hpp
    ${MOTION_APP_DIR}/Scene/ScenePanels/PropertiesPanel.hpp
    ${MOTION_APP_DIR}/Scene/ScenePanels/ScenePanel.hpp
    ${MOTION_APP_DIR}/Scene/ScenePanels/ProjectSceneViewPanel.hpp
    ${MOTION_APP_DIR}/Scene/ScenePanels/Panels.hpp
)

set(
    MOTION_APP_SOURCES
    ${MOTION_APP_DIR}/EntryPoint/MotionEngine.cpp
    ${MOTION_APP_DIR}/EntryPoint/Application.cpp
    ${MOTION_APP_DIR}/Layers/ImGuiLayer.cpp
    ${MOTION_APP_DIR}/Layers/SceneEditorLayer.cpp
    ${MOTION_APP_DIR}/Scene/Scene.cpp
    ${MOTION_APP_DIR}/Scene/SceneCamera.cpp
    ${MOTION_APP_DIR}/Scene/SceneRenderer.cpp
    ${MOTION_APP_DIR}/Scene/SceneEnviroment.cpp

    ${MOTION_APP_DIR}/Scene/ScenePanels/ViewportPanel.cpp
    ${MOTION_APP_DIR}/Scene/ScenePanels/SettingsPanel.cpp
    ${MOTION_APP_DIR}/Scene/ScenePanels/EntityInspectPanel.cpp
    ${MOTION_APP_DIR}/Scene/ScenePanels/PropertiesPanel.cpp
    ${MOTION_APP_DIR}/Scene/ScenePanels/ProjectSceneViewPanel.cpp
)

add_executable(${MOTION_APP_EXECUTABLE} ${MOTION_APP_HEADERS} ${MOTION_APP_SOURCES})
target_include_directories(${MOTION_APP_EXECUTABLE} PUBLIC ${MTION_APP_INCLUDE_DIR} ${MOTION_CORE_INCLUDE_DIR})
target_link_libraries(${MOTION_APP_EXECUTABLE} PUBLIC ${MOTION_CORE_LIBRARY})

add_custom_target(
    MotionEngineResources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/Assets
    ${PROJECT_BINARY_DIR}/Assets
    COMMENT "Copying resources into the binary dir"
)
add_dependencies(${MOTION_APP_EXECUTABLE} MotionEngineResources)