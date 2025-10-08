#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "Scene.hpp"
#include "Components.hpp"
#include "SceneEnviroment.hpp"

namespace Motion
{
    class SceneSerializer
    {
        public:
            static bool Serialize(Scene* scene, const std::filesystem::path& path);
            static std::shared_ptr<Scene> Deserialize(const std::filesystem::path& path);
            
            static bool SerializeRuntime(Scene* scene, const std::filesystem::path& path);
            static bool DeserializeRuntime(Scene* scene, const std::filesystem::path& path);
    };
}