#pragma once

//std
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <functional>
#include <type_traits>
#include <algorithm>
#include <utility>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <bitset>
#include <future>
#include <thread>
#include <regex>
#include <mutex>
#include <format>

//external
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <stb/stb_image.h>
#include <stb/stb_image_resize2.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/util.hpp>

#include <MikkTSpace/mikktspace.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/intersect.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_internal.h>
#include <imguizmo/ImGuizmo.h>

#include <yaml-cpp/yaml.h>
#include <MaterialFonts/IconsMaterialDesign.h>

//Base
#include "Base.hpp"
#include "Asserts.hpp"
#include "Log.hpp"
#include "UUID.hpp"

//Layers
#include "Layer.hpp"
#include "LayersManager.hpp"

// Assets
#include "Asset.hpp"
#include "AssetManager.hpp"

//Material
#include "Material.hpp"

//Maths
#include "Intersect.hpp"

//Events
#include "Event.hpp"
#include "EventKeyboard.hpp"
#include "EventMouse.hpp"
#include "EventWindow.hpp"
#include "EventRegistry.hpp"
#include "InputsHandler.hpp"
#include "GLFW_InputsHandler.hpp"

//Renderer
#include "Buffers.hpp"
#include "Shaders.hpp"
#include "Texture.hpp"
#include "Arrays.hpp"
#include "Camera.hpp"
#include "Camera3D.hpp"
#include "Model.hpp"
#include "Mesh.hpp"
#include "Importer.hpp"
#include "Thumbnail.hpp"
#include "PostProcessor.hpp"
#include "Environment.hpp"
#include "Renderer.hpp"

//Renderer - OpenGL
#include "GL_Debug.hpp"
#include "GL_Renderer.hpp"
#include "GL_Buffers.hpp"
#include "GL_Shaders.hpp"
#include "GL_Texture.hpp"
#include "GL_Arrays.hpp"
#include "GL_Environment.hpp"
#include "GLFW_GL_Context.hpp"

//Window
#include "Window.hpp"
#include "GLFW_Window.hpp"

//UserInterfaceInitializer
#include "UI.hpp"

//ECS
#include "Entity.hpp"
#include "Components.hpp"

//Platform
#include "DialogBoxes.hpp"