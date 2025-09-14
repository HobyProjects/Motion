cmake_minimum_required(VERSION 3.24)
project(Motion VERSION 1.0.0 LANGUAGES C CXX)

# Outputs
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Dependencies (target-based)
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

# Core library
add_library(MotionCore STATIC)
target_compile_features(MotionCore PUBLIC cxx_std_20)
target_include_directories(
    MotionCore 
        PUBLIC 
        ${CMAKE_SOURCE_DIR}/Src/Core
        ${CMAKE_SOURCE_DIR}/Src/Core/Base
        ${CMAKE_SOURCE_DIR}/Src/Core/Event
        ${CMAKE_SOURCE_DIR}/Src/Core/Window
        ${CMAKE_SOURCE_DIR}/Src/Core/Assets
        ${CMAKE_SOURCE_DIR}/Src/Core/Layer
        ${CMAKE_SOURCE_DIR}/Src/Core/UI
        ${CMAKE_SOURCE_DIR}/Src/Core/UI/IconFonts
        ${CMAKE_SOURCE_DIR}/Src/Core/ECS
        ${CMAKE_SOURCE_DIR}/Src/Core/Material
        ${CMAKE_SOURCE_DIR}/Src/Core/Model
        ${CMAKE_SOURCE_DIR}/Src/Core/Maths
        ${CMAKE_SOURCE_DIR}/Src/Core/Physics
        ${CMAKE_SOURCE_DIR}/Src/Core/Physics/Broadphase
        ${CMAKE_SOURCE_DIR}/Src/Core/Physics/Collision
        ${CMAKE_SOURCE_DIR}/Src/Core/Physics/PhyCore
        ${CMAKE_SOURCE_DIR}/Src/Core/Physics/Primitives
        ${CMAKE_SOURCE_DIR}/Src/Core/Physics/Query
        ${CMAKE_SOURCE_DIR}/Src/Core/Physics/Solver
        ${CMAKE_SOURCE_DIR}/Src/Core/Platform
        ${CMAKE_SOURCE_DIR}/Src/Core/Renderer
        ${CMAKE_SOURCE_DIR}/Src/Core/Renderer/Backends/OpenGL
)

file(GLOB_RECURSE CORE_CPP CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/Src/Core/*.cpp")
file(GLOB_RECURSE CORE_HPP CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/Src/Core/*.h" "${CMAKE_SOURCE_DIR}/Src/Core/*.hpp")
target_sources(MotionCore PRIVATE ${CORE_CPP} PUBLIC FILE_SET HEADERS BASE_DIRS "${CMAKE_SOURCE_DIR}/Src/Core" FILES ${CORE_HPP})

target_link_libraries(MotionCore
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

# PCH (optional)
target_precompile_headers(MotionCore PUBLIC Src/Core/Base/CorePCH.hpp)

# App
add_executable(MotionEngine)
target_compile_features(MotionEngine PRIVATE cxx_std_20)
target_include_directories(MotionEngine PRIVATE ${CMAKE_SOURCE_DIR}/Src/App)
file(GLOB_RECURSE APP_CPP CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/Src/App/*.cpp")
file(GLOB_RECURSE APP_HPP CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/Src/App/*.h" "${CMAKE_SOURCE_DIR}/Src/App/*.hpp")
target_sources(MotionEngine PRIVATE ${APP_CPP} ${APP_HPP})
target_link_libraries(MotionEngine PRIVATE MotionCore)

# Assets copy
add_custom_target(MotionEngineResources ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${PROJECT_SOURCE_DIR}/Assets
          ${PROJECT_BINARY_DIR}/Assets
  COMMENT "Copying resources into the binary dir"
)
add_dependencies(MotionEngine MotionEngineResources)
