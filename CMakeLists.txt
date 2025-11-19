cmake_minimum_required(VERSION 3.24)
project(Motion VERSION 1.0.0 LANGUAGES C CXX)

# Version control
include(${CMAKE_SOURCE_DIR}/cmake/GenerateVersionHeader.cmake)
set(VERSION_HEADER_FILE "${CMAKE_BINARY_DIR}/generated/MotionVersion.hpp")
generate_version_header(
    TARGET MotionCore
    VERSION_FILE ${VERSION_HEADER_FILE}
)

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
find_package(MikkTSpace CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(reactphysics3d CONFIG REQUIRED)

# Core library
add_library(MotionCore STATIC)
target_compile_definitions(MotionCore PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_features(MotionCore PUBLIC cxx_std_20)
target_include_directories(
    MotionCore 
        PUBLIC 
        ${CMAKE_BINARY_DIR}/generated
        ${CMAKE_SOURCE_DIR}/Src/Core
        ${CMAKE_SOURCE_DIR}/Src/Core/Base
        ${CMAKE_SOURCE_DIR}/Src/Core/Event
        ${CMAKE_SOURCE_DIR}/Src/Core/Window
        ${CMAKE_SOURCE_DIR}/Src/Core/Assets
        ${CMAKE_SOURCE_DIR}/Src/Core/Layer
        ${CMAKE_SOURCE_DIR}/Src/Core/UI
        ${CMAKE_SOURCE_DIR}/Src/Core/UI/IconFonts
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
    MikkTSpace::MikkTSpace
    imguizmo::imguizmo
    ReactPhysics3D::ReactPhysics3D
)

# PCH (optional)
target_precompile_headers(MotionCore PUBLIC Src/Core/Base/CorePCH.hpp)

# App
add_executable(MotionEngine)
target_compile_definitions(MotionEngine PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_features(MotionEngine PRIVATE cxx_std_20)
target_include_directories(
    MotionEngine 
        PUBLIC 
        ${CMAKE_SOURCE_DIR}/Src/App
        ${CMAKE_SOURCE_DIR}/Src/App/EntryPoint
        ${CMAKE_SOURCE_DIR}/Src/App/Layers
        ${CMAKE_SOURCE_DIR}/Src/App/Scene
        ${CMAKE_SOURCE_DIR}/Src/App/Scene/Serializer
        ${CMAKE_SOURCE_DIR}/Src/App/Scene/Panels
)
file(GLOB_RECURSE APP_CPP CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/Src/App/*.cpp")
file(GLOB_RECURSE APP_HPP CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/Src/App/*.h" "${CMAKE_SOURCE_DIR}/Src/App/*.hpp")
target_sources(MotionEngine PRIVATE ${APP_CPP} ${APP_HPP})
target_link_libraries(MotionEngine PRIVATE MotionCore)

# Track asset files and run copy via a helper target
file(GLOB_RECURSE ASSET_FILES "${CMAKE_SOURCE_DIR}/Assets/*")

add_custom_target(MotionEngineAssets ALL
  COMMAND ${CMAKE_COMMAND} -E echo "Syncing Assets -> $<TARGET_FILE_DIR:MotionEngine>/Assets"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_SOURCE_DIR}/Assets"
          "$<TARGET_FILE_DIR:MotionEngine>/Assets"
  DEPENDS ${ASSET_FILES}
  VERBATIM
)

add_dependencies(MotionEngine MotionEngineAssets)